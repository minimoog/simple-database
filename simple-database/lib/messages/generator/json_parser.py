#!/usr/bin/python3
import sys, json, getopt, os
import message_utils

argument_types = [
    'bool',
    'uint8_t',
    'uint16_t',
    'uint32_t',
    'uint64_t',
    'string',
    'internal_buffer',
    'external_buffer',
    'const_length_buffer',
    'uint256',
    'length_encoded_set',
    'string_set',
    'primitive_set',
    'serializeable_set',
    'enum_set',
    'enum',
    'serializeable',
]

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def load(f):
    try:
        with open(f) as json_data:
            try:
                d = json.load(json_data)
            except json.decoder.JSONDecodeError as e:
                eprint("Invalid json in %s" %(f))
                eprint(e)
                sys.exit(255)

            return d
    except FileNotFoundError as e:
        eprint("Unable to load schema file: %s" %(f))
        eprint(e)
        sys.exit(255)


def validate(messages, enums, constants):
    names = gather_names(messages)
    _ = gather_names(enums, type="enum")

    for i, enum in enumerate(enums):
        name = enum['name']

        if name in names:
            enum_error(enum, i, "Enum shares a name with a message, this is not allowed")

        try:
            values = enum['enum']
        except KeyError:
            enum_error(enum, i, "Enum must have a key 'enum' which is a list of names")

        if len(name) > constants["EnumMaxLength"]:
            enum_error(enum, i, "Enum string length must be less than or equal to %s" %(constants["EnumMaxLength"]))

        if not isinstance(values, list):
            enum_error(enum, i, "Enum must have a key 'enum' which is a list of names")

        for e in values:
            if e[:1].islower() or " " in e or not e.isalpha():
                enum_error(enum, i, "enum values must be pascal case (LikeThis)")

    for i, message in enumerate(messages):
        if len(message["name"]) > constants["EnumMaxLength"]:
            message_error(message, i, "Message name string length must be less than or equal to %s" %(constants["EnumMaxLength"]))

        if 'ifdef' in message:
            allowed = ['CRYPTOCURRENCY', 'CONFIDENTIAL_COMPUTE']
            if not message['ifdef'] in allowed:
                message_error(message, i, "Message contains unknown 'ifdef' (%s). "
                        "Should be one of: %s" %(message['ifdef'], ", ".join(allowed)))

        if 'properties' in message:
            props = message['properties']
        else:
            continue

        prop_names = []
        for propNum, prop in enumerate(props):
            try:
                pname = prop['name']
            except KeyError:
                property_error(message, i, prop, "property must have a name")

            if pname in names:
                property_error(message, i, prop, "a property cannot share a name with a message")

            if pname in prop_names:
                property_error(message, i, prop, "property appears twice in message")
            prop_names += [pname]

            if pname[:1].islower() or " " in pname or not pname.isalpha():
                property_error(message, i, prop, "property name is not title case (LikeThis)")

            try:
                ptype = prop['type']
            except KeyError:
                property_error(message, i, prop, "property must have a type")

            # id means uint64_t
            if ptype == "id":
                ptype = "uint64_t"
                prop["type"] = "uint64_t"

            if ptype not in argument_types:
                property_error(message, i, prop, "%s is an unknown or unsupported type" %(ptype))

            if ptype == "serializeable":
                try:
                    if prop['reference'] not in names:
                        property_error(message, i, prop, "Unknown reference: %s - this should be the name of another type" %(prop['reference']))
                except KeyError:
                    property_error(message, i, prop, "A serializeable must contain a 'reference' key")
            if message_utils.type_is_set(ptype):
                try:
                    item_number = prop['item_number']
                except KeyError:
                    property_error(message, i, prop, "set property requires a maximum number of items")

                try:
                    in_name = item_number['name']
                except KeyError:
                    property_error(message, i, prop, "property type is a set, but does not have an item number descriptor name")

                if in_name[:1].islower() or " " in in_name or not in_name.isalpha():
                    property_error(message, i, prop, "property item_number descriptor name is not title case (LikeThis)")

                if in_name not in constants:
                    try:
                        in_size = item_number['value']
                    except KeyError:
                        property_error(message, i, prop, "property type is set, but does not have an item number value")

                    if not isinstance(in_size, int):
                        property_error(message, i, prop, "property type is set, but does not have a valid item number max")

                if ptype == "primitive_set":
                    try:
                        item_type = prop['item_type']
                    except KeyError:
                        property_error(message, i, prop, "property requires an item type")

                    allowed_types = ["bool", "uint8_t", "uint16_t", "uint32_t", "uint64_t"]
                    if item_type not in allowed_types:
                        property_error(message, i, prop, "property requires item_type to be one of: " + allowed_types.join(', '))
                elif ptype == "serializeable_set":
                    try:
                        if prop['reference'] not in names:
                            property_error(message, i, prop, "Unknown reference: %s - this should be the name of another type" %(prop['reference']))
                    except KeyError:
                        property_error(message, i, prop, "A serializeable_set must contain a 'reference' key")
                elif ptype != "enum_set":
                    try:
                        item_length = prop['item_length']
                    except KeyError:
                        property_error(message, i, prop, "property requires a maximum item length")

                    try:
                        il_name = item_length['name']
                    except KeyError:
                        property_error(message, i, prop, "property type is length encoded set, but does not have an item length descriptor name")

                    if il_name[:1].islower() or " " in il_name or not il_name.isalpha():
                        property_error(message, i, prop, "property item_length descriptor name is not title case (LikeThis)")

                    if il_name not in constants:
                        try:
                            il_size = item_length['value']
                        except KeyError:
                            property_error(message, i, prop, "property type is length encoded set, but does not have an item length value")

                        if not isinstance(il_size, int):
                            property_error(message, i, prop, "property type is length encoded set, but does not have a valid item length max")

            if ptype in ["string", "internal_buffer", "external_buffer", "const_length_buffer"]:
                try:
                    psize = prop['size']
                except KeyError:
                    property_error(message, i, prop, "property type is variable length, but does not have a size description")

                try:
                    psize_name = psize['name']
                except KeyError:
                    property_error(message, i, prop, "property type is variable length, but does not have a size descriptor name")

                if psize_name[:1].islower() or " " in psize_name:
                    property_error(message, i, prop, "property size descriptor name is not title case (LikeThis)")

                if psize_name not in constants:
                    try:
                        psize_value = psize['value']
                    except KeyError:
                        property_error(message, i, prop, "property type is variable length, but does not have a max size")

                    if propNum == len(props) - 1 and psize_value == "remainder":
                        pass
                    elif not isinstance(psize_value, int):
                        property_error(message, i, prop, "property type is variable length, but does not have a valid max size")
            if ptype == "uint256":
                prop["type"] = "uint256"
            if ptype == "enum" or ptype == "enum_set":
                if 'enum' not in prop and 'global' not in prop:
                    property_error(message, i, prop, "property type is enum, but does not have enum array, or a global to a global enum")

                if 'enum' in prop and 'global' in prop:
                    property_error(message, i, prop, "enum property must only define an enum or a global key")

                if 'enum' in prop:
                    penum = prop['enum']
                    if not isinstance(penum, list):
                        property_error(message, i, prop, "enum key of enum property should be a list of names")
                    for e in penum:
                        if e[:1].islower() or " " in e or not e.isalpha():
                            property_error(message, i, prop, "enum values must be pascal case (LikeThis)")

                if 'global' in prop:
                    if prop['global'] != 'MessageType' and prop['global'] not in [x['name'] for x in enums]:
                        property_error(message, i, prop, "Unknown global: %s - this should be the name of an Enum" %(prop['global']))



def gather_names(json, type="message"):
    names = []
    for i, message in enumerate(json):
        try:
            name = message['name']
        except KeyError:
            eprint('Message at index {} does not appear to have a name.\n{}'.format(i, message))
            sys.exit(255)

        if name in names:
            if type == "message":
                message_error(message, i, "Message name appears twice.")
            else:
                enum_error(message, i, "Enum name appears twice.")
        names += [name]
    return names

def enum_error(enum, index, error):
    eprint("Enum at index: %d, %s is not valid" %(index, enum['name']))
    eprint(error)
    sys.exit(255)

def property_error(message, index, prop, error):
    eprint("Message at index: %d, %s is not valid" %(index, message['name']))
    eprint("property \"%s\" is invalid" % (prop['name']))
    eprint(error)
    sys.exit(255)

def message_error(message, index, error):
    eprint("Message at index: %d, %s is not valid" %(index, message['name']))
    eprint(error)
    sys.exit(255)

