import argparse
import datetime
import os
import sys
import hashlib
import json
import re
import shutil
import subprocess

import json_parser
import property_generator
import message_utils
from statement_builder import builder

ID_PROP = { "name": "Id", "type": "uint64_t" }
TABLE_META = { "name": "Metadata", "type": "serializeable", "reference": "TableMeta" }

def main(schema_dir, output_dir, dry_run=False):

    serializeable_files = [
        os.path.join(schema_dir, "Serializeable.json"),
        os.path.join(schema_dir, "Events.json"),
        os.path.join(schema_dir, "Tables.json"),
        os.path.join(schema_dir, "Messages.json"),
        ]

    all_messages = []

    for fnum, definitions in enumerate(serializeable_files):
        messages = json_parser.load(definitions)
        for m in messages:
            m['meta_type'] = {
                0: "serializeable",
                1: "event",
                2: "table",
                3: "message",
            }[fnum]

            if fnum == 2:
                m['properties'].insert(0, ID_PROP)
                m['properties'].append(TABLE_META)
            all_messages.append(m)

    enums = json_parser.load(os.path.join(schema_dir, "Enums.json"))
    constants = json_parser.load(os.path.join(schema_dir, "Constants.json"))

    json_parser.validate(all_messages, enums, constants)


    # hash all the schema files after stripping all the whitespace
    ws_pattern = re.compile(r'\s+')
    hasher = hashlib.md5()

    hash_json = lambda x: hasher.update(bytes(re.sub(ws_pattern, '', x.read()), 'utf-8'))
    for file in serializeable_files:
        with open(file) as data_to_hash:
            hash_json(data_to_hash)
    with open(os.path.join(schema_dir, "Enums.json")) as data_to_hash:
        hash_json(data_to_hash)
    with open(os.path.join(schema_dir, "Constants.json")) as data_to_hash:
        hash_json(data_to_hash)

    version_hash = hasher.digest()

    calculate_field_sizes(all_messages, constants)
    calculate_max_sizes(all_messages)

    if not dry_run:
        clean_output_folder(output_dir)

    def_files = create_definitions(all_messages, enums, constants, version_hash)

    outputs = []

    for d in def_files:
        outputs += [save_definition_file(output_dir, d, dry_run)]

    for m in all_messages:
        files = create_files(m)
        for f in files:
            if f['skip_empty'] and not ('properties' in m and len(m["properties"])):
                continue
            f = insert_class_info(f, m)
            f = property_generator.insert_properties(f, m, constants)
            f = insert_license(f, m)
            f = trim(f, m)

            outputs += [save_message_file(output_dir, f, m, dry_run)]

    if dry_run:
        sys.stdout.write(';'.join(outputs))

def size_of(msg):
    s = 0
    try:
        for prop in msg["properties"]:
            s += prop["field_size"] if "field_size" in prop else 0
    except KeyError:
        pass

    return s


def calculate_max_sizes(all_msgs):
    '''
    Calculate the max serialized size each message can be
    This should be called after `calculate_field_sizes`
    '''
    for msg in all_msgs:
        msg['max_size'] = size_of(msg)


def calculate_field_sizes(all_msgs, constants):
    '''
    Calculate the sizes all message properties
    Warning: do not call this more than once
             it fail to calculate remainder fields on subsequent calls
    '''
    calculated = []
    while len(calculated) != len(all_msgs):
        for msg in all_msgs:

            if msg["name"] in calculated:
                continue

            calculated.append(msg["name"])

            if "properties" not in msg:
                continue

            for i, prop in enumerate(msg["properties"]):
                size = {
                        "bool": 1,
                        "uint8_t": 1,
                        "uint16_t": 2,
                        "uint32_t": 4,
                        "uint64_t": 8,
                        "uint256" : 32,
                        }.get(prop["type"], 0)

                if size == 0:
                    if prop["type"] == "serializeable":
                        other_msg = [m for m in all_msgs if m["name"] == prop["reference"]][0]
                        size = size_of(other_msg)
                        if size == 0 and "properties" in other_msg:
                            # come back later, maybe we didn't calculate that one yet
                            calculated.pop()
                            break

                    if message_utils.type_is_set(prop["type"]):
                        per_item_l = 0
                        if prop["type"] == "string_set" or prop["type"] == "enum_set":
                            per_item_l = 1
                        if prop["type"] == "length_encoded_set":
                            per_item_l = 4

                        if prop["item_number"]["name"] in constants:
                            item_n_val = int(constants[prop["item_number"]["name"]])
                        else:
                            item_n_val = prop["item_number"]["value"]

                        if prop["type"] == "enum_set":
                            item_l_val = int(constants["EnumMaxLength"])
                        else:
                            try:
                                if prop["item_length"]["name"] in constants:
                                    item_l_val = int(constants[prop["item_length"]["name"]])
                                else:
                                    item_l_val = prop["item_length"]["value"]
                            except KeyError:
                                if prop["type"] == "serializeable_set":
                                    other_msg = [m for m in all_msgs if m["name"] == prop["reference"]][0]
                                    item_l_val = size_of(other_msg)
                                    if item_l_val == 0 and "properties" in other_msg:
                                        # come back later, maybe we didn't calculate that one yet
                                        calculated.pop()
                                        break
                                elif prop["item_type"] == "uint64_t":
                                    item_l_val = 8
                                elif prop["item_type"] == "uint32_t":
                                    item_l_val = 4
                                elif prop["item_type"] == "uint16_t":
                                    item_l_val = 2
                                elif prop["item_type"] == "uint8_t":
                                    item_l_val = 1
                                elif prop["item_type"] == "bool":
                                    item_l_val = 1

                        size = item_n_val * (item_l_val + per_item_l) + 4

                    if prop["type"] in ["string", "internal_buffer", "external_buffer", "const_length_buffer"]:
                        if prop["size"]["name"] in constants:
                            size = int(constants[prop["size"]["name"]])
                        else:
                            size = prop["size"]["value"]
                        if size == "remainder":
                            size = constants["MessageLength"] - constants["HeaderMinLength"] - constants["EnumMaxLength"] - size_of(msg)
                        elif prop["type"] == "string":
                            size += 1
                        elif prop["type"] in ["internal_buffer", "external_buffer"]:
                            size += 4

                    if prop["type"] == "enum":
                        size = int(constants["EnumMaxLength"]) + 1


                msg["properties"][i]["field_size"] = size


def create_files(msg):
    files = []

    for t in os.listdir(template_dir()):
        temp_name = os.path.join(template_dir(), t)
        if os.path.isdir(temp_name):
            continue
        if t == "license-header.txt":
            continue
        if t.endswith(".swp"):  # stupid vim
            continue

        # don't make a message class if the meta type is "serializeable"
        if "serializeable" not in t and msg["meta_type"] == "serializeable":
            continue

        with open(temp_name) as f:
            if len(t.split(".")) > 1:
                ext = t.split(".")[1]
            else:
                ext = ""

            # append 'Message' to generated files
            # except for serializeable classes
            # its exceptions like this that make me sad
            # :crying_cat_meme:
            append = "Message"
            skip_empty = False
            if "serializeable" in t:
                append = ""
                skip_empty = True

            files += [{"text": f.read(), "ext": ext, "append": append, "skip_empty": skip_empty}]

    return files


def template_dir():
    return os.path.join(os.path.dirname(__file__), "templates")

def create_definitions(msgs, enums, constants, version_hash):
    templates = os.path.join(template_dir(), "definitions")
    defs = []
    for temp_file_name in os.listdir(templates):
        temp_name = os.path.join(templates, temp_file_name)
        if os.path.isdir(temp_name):
            continue

        def_file = ""
        with open(temp_name) as f:
            def_file = f.read()

        num_spaces = 0
        p = re.compile("\s*\$\$EVENT_TYPES\$\$")
        m = p.search(def_file)
        if m:
            line = def_file[m.span()[0]:m.span()[1]].replace("\n", "")
            num_spaces = len(line) - len(line.lstrip())
        joiner = "\n" + (" " * num_spaces)

        # Version hash
        vhex = version_hash.hex()
        def_file = def_file.replace("$$VERSION_HASH$$", vhex)

        byte_guy = "0x"
        vbytes = (", " + byte_guy).join([vhex[i:i+2] for i in range(0, len(vhex), 2)])
        vbytes = byte_guy + vbytes
        def_file = def_file.replace("$$VERSION_HASH_BYTE_ARRAY$$", vbytes)

        # Message type definitions
        for meta, placeholder, offset in [
                ("event", "$$EVENT_TYPES$$", 0),
                ("table", "$$TABLE_TYPES$$", 1000),
                ("message", "$$MESSAGE_TYPES$$", 2000),
                ("serializeable", "$$SERIALIZEABLE_TYPES$$", 3000),
                ]:
            names = []
            meta_types = [x for x in msgs if x["meta_type"] == meta]
            for m in meta_types:
                names.append(m["name"])

            cpp_types = list(map(lambda n: n + "Type,", names))
            def_file = def_file.replace(placeholder, joiner.join(cpp_types))

        # Message Type loop
        loop_split = def_file.split("%%TYPE_LOOP%%")

        if len(loop_split) < 3:
            def_file = def_file.replace("%%TYPE_LOOP%%", "")
        else:
            if len(loop_split) % 2 == 0:
                json_parser.eprint("definition template %s has an odd number of %%TYPE_LOOP%% markers" %(temp_name))
                sys.exit(255)

            def_file = ""
            for i in range(1, len(loop_split) - 1, 2):
                loop_part = ""
                for m in msgs:
                    loop_part += loop_split[i] \
                        .replace("$$MESSAGE_TYPE$$", m["name"]) \
                        .replace("$$MESSAGE_NAME$$", m["name"])

                def_file += loop_split[i - 1] + loop_part

            def_file += loop_split[-1]

        # Serializeable loop
        for marker, messages in [
            ("%%MESSAGE_WITH_ID_LOOP%%", [m for m in msgs if not "ifdef" in m]),
            ("%%CC_MESSAGE_WITH_ID_LOOP%%", [m for m in msgs if "ifdef" in m and m["ifdef"] == "CONFIDENTIAL_COMPUTE"]),
            ("%%CRYPTO_MESSAGE_WITH_ID_LOOP%%", [m for m in msgs if "ifdef" in m and m["ifdef"] == "CRYPTOCURRENCY"]),
        ]:
            loop_split = def_file.split(marker)

            if len(loop_split) < 3:
                def_file = def_file.replace(marker, "")
            else:
                if len(loop_split) % 2 == 0:
                    json_parser.eprint("definition template %s has an odd number of %%MESSAGE_WITH_ID_LOOP%% markers" %(temp_name))
                    sys.exit(255)

                def_file = ""
                for i in range(1, len(loop_split) - 1, 2):
                    loop_part = ""
                    for m in [
                        m for m in messages if "properties" in m and
                            len([p for p in m["properties"] if p["name"] == "Id"])
                    ]:
                        loop_part += loop_split[i] \
                            .replace("$$MESSAGE_TYPE$$", m["name"]) \
                            .replace("$$MESSAGE_NAME$$", m["name"])

                    def_file += loop_split[i - 1] + loop_part

                def_file += loop_split[-1]

        # Const loop
        const_split = def_file.split("%%CONSTANT_LOOP%%")

        if len(const_split) < 3:
            def_file = def_file.replace("%%CONSTANT_LOOP%%", "")
        else:
            if len(loop_split) % 2 == 0:
                json_parser.eprint("definition template %s has an odd number of %%CONSTANT_LOOP%% markers" %(temp_name))
                sys.exit(255)

            def_file = ""
            for i in range(1, len(const_split) - 1, 2):
                loop_part = ""
                for m in constants:
                    loop_part += const_split[i] \
                        .replace("$$CONSTANT_NAME$$", m) \
                        .replace("$$CONSTANT_VALUE$$", str(constants[m]))

                def_file += const_split[i - 1] + loop_part

            def_file += const_split[-1]

        # Global enums
        def_file = builder.global_enums(enums, def_file)

        defs += [{"text": def_file, "name": temp_file_name}]

    return defs


def insert_class_info(f, msg):
    f['text'] = f['text'].replace("$$IFDEF$$", msg['ifdef'] if 'ifdef' in msg else '1' )
    f['text'] = f['text'].replace("$$CLASS_NAME$$", msg['name'] + "Message")

    f['text'] = f['text'].replace("$$CLASS_NAME_UPPER$$", msg['name'].upper() + "MESSAGE")

    base_name = msg['name'] if 'properties' in msg and len(msg['properties']) else 'Serializeable'
    f['text'] = f['text'].replace("$$BASE_NAME_UPPER$$", base_name.upper())
    f['text'] = f['text'].replace("$$BASE_NAME$$", base_name)

    if "properties" in msg:
        to_include = [p for p in msg["properties"] if "reference" in p]
        if len(to_include):
            includes = list(map(lambda m: "#include \"" + m['reference'] + ".hpp\"\n", to_include))
            f['text'] = f['text'].replace("$$INCLUDED_SERIALIZEABLES$$", ''.join(includes))
        else:
            f['text'] = f['text'].replace("$$INCLUDED_SERIALIZEABLES$$", '')
    else:
        f['text'] = f['text'].replace("$$INCLUDED_SERIALIZEABLES$$", '')

    try:
        base_class = msg['base_class']
    except KeyError:
        base_class = "BaseMessage"

    if base_class == "BaseMessage":
        f['text'] = f['text'].replace("$$BASE_IMPORT$$\n", "")

    f['text'] = f['text'].replace("$$BASE_CLASS$$", base_class)

    f['text'] = f['text'].replace("$$MESSAGE_TYPE$$", msg['name'] + "Type")

    friends = ""
    if 'friends' in msg:
        for friend in msg['friends']:
            friends += "friend class %s;\n" % (friend)
    f['text'] = f['text'].replace("$$SERIALIZEABLE_FRIEND_CLASSES$$", friends)

    return f


def insert_license(f, msg):
    with open(os.path.join(template_dir(), "license-header.txt")) as license_file:
        license = license_file.read()

    license = license.replace("$$FILE_NAME$$",
                              msg['name'] + "Message." + f['ext'])

    license = license.replace("$$YEAR$$", str(datetime.datetime.now().year))
    f["text"] = license + f["text"]

    return f


def save_definition_file(output_dir, d, dry_run):
    msg_dir = os.path.join(output_dir, "definitions")

    if not os.path.exists(msg_dir):
        os.makedirs(msg_dir)

    filename = os.path.join(msg_dir, d["name"])

    if not dry_run:
        d["text"] = clang_format(d["text"])
        with open(filename, 'w') as out:
            out.write(d["text"])

    return filename


def save_message_file(output_dir, f, msg, dry_run):
    f["text"] = clang_format(f["text"])

    msg_dir = os.path.join(output_dir, msg["meta_type"])
    if not os.path.exists(msg_dir):
        os.makedirs(msg_dir)

    filename = os.path.join(msg_dir, msg["name"] + f["append"] + "." + f["ext"])

    if not dry_run:
        with open(filename, 'w') as out:
            out.write(f["text"])

    return filename

def clean_output_folder(output_dir):
    for root, dirs, files in os.walk(output_dir):
        for file in files:
            os.remove(os.path.join(root, file))

        for dir in dirs:
            shutil.rmtree(dir, ignore_errors=True)


def trim(f, msg):
    f["text"] = f["text"].replace("\n\n\n", "\n\n")
    if f["ext"] == "hpp":
        f["text"] = f["text"].replace(msg["name"] + "Message::", "")
        f["text"] = f["text"].replace("public:\n    \n\n", "public:\n")
    return f


def clang_format(file_text):
    try:
        formatter = subprocess.Popen(["clang-format", "-style=file"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        formatted = formatter.communicate(input=file_text.encode())[0]
        output = formatted.decode()
    except FileNotFoundError:
        output = file_text

    return file_text

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("schema_dir", help="directory containing the json definitions")
    parser.add_argument("output_dir", help="directory to output the generated code")

    parser.add_argument("--dry-run",
                        help="print the source files that will be generated, delimited by ';' (for use by CMake)",
                        action="store_true")
    args = parser.parse_args()
    main(args.schema_dir, args.output_dir, args.dry_run)
