import re
import sys
from json_parser import eprint

class builder():
    def __init__(self, message, constants):
        self.msg = message
        self.constants = constants


    def global_enums(enums, template):
        enumstr = template

        enum_loop_split = template.split("%%ENUM_LOOP%%")

        if len(enum_loop_split) < 3:
            enumstr = template.replace("%%ENUM_LOOP%%", "")
        else:
            if len(enum_loop_split) % 2 == 0:
                eprint("enum template has an odd number of %%ENUM_LOOP%% markers")
                sys.exit(255)

            enumstr = ""
            for i in range(1, len(enum_loop_split) - 1, 2):
                enum_loop_part = ""
                for enum in enums:
                    enum_value_loop_split = enum_loop_split[i].split("%%ENUM_VALUE_LOOP%%")
                    if len(enum_value_loop_split) < 3:
                        continue
                    else:
                        if len(enum_value_loop_split) % 2 == 0:
                            eprint("enum template has an odd number of %%ENUM_VALUE_LOOP%% markers")
                            sys.exit(255)
                    for j in range(1, len(enum_value_loop_split) - 1, 2):
                        next_enum_loop_part = ""
                        for val in enum["enum"]:
                            next_enum_loop_part += enum_value_loop_split[j].replace("$$ENUM_VALUE$$", val)

                        enum_loop_part += enum_value_loop_split[j - 1] + next_enum_loop_part
                    enum_loop_part += enum_value_loop_split[-1]
                    enum_loop_part = enum_loop_part.replace("$$ENUM_NAME$$", enum["name"])
                    enum_loop_part = enum_loop_part.replace("$$ENUM_SIZE$$", str(len(enum["enum"])))

                enumstr += enum_loop_split[i - 1] + enum_loop_part

            enumstr += enum_loop_split[-1]

        return enumstr

    def const_definitions(self, prop):
        string = ""
        if "size" in prop and prop["size"]["name"] not in self.constants:
            size = prop["field_size"]
            if prop["type"] == "string":
                size -= 1
            if prop["type"] in ["internal_buffer", "external_buffer"]:
                size -= 4
            string += "static const uint32_t %s = %s;\n" %(prop["size"]["name"], size)

        if "item_length" in prop and prop["item_length"]["name"] not in self.constants:
            string += "static const uint32_t %s = %s;\n" %(prop["item_length"]["name"], prop["item_length"]["value"])

        if "item_number" in prop and prop["item_number"]["name"] not in self.constants:
            string += "static const uint32_t %s = %s;\n" %(prop["item_number"]["name"], prop["item_number"]["value"])

        return string

    def property_type(self, prop):
        if 'enum' in prop:
            return self.msg["name"] + "::" +  prop["name"] + "_t"
        elif 'global' in prop:
            return prop['global']
        else:
            return prop["type"]

    def get_set(self, prop, template):
        get_set = template

        get_set = get_set.replace("$$TYPE$$", self.property_type(prop))
        get_set = get_set.replace("$$BASE_NAME$$", self.msg["name"])
        get_set = get_set.replace("$$PROPERTY_NAME$$", prop["name"])
        get_set = get_set.replace("$$ARG$$", prop["name"][:1].lower() + prop["name"][1:])
        get_set = get_set.replace("$$VARIABLE$$", "m" + prop["name"])
        return get_set

    def get_set_header(self, prop, template):
        get_set_h = template

        get_set_h = get_set_h.replace("$$TYPE$$", self.property_type(prop))
        get_set_h = get_set_h.replace("$$PROPERTY_NAME$$", prop["name"])
        get_set_h = get_set_h.replace("$$ARG$$", prop["name"][:1].lower() + prop["name"][1:])
        get_set_h = get_set_h.replace("$$VARIABLE$$", "m" + prop["name"])
        return get_set_h

    def enum(self, prop, template):
        if 'enum' not in prop:
            return

        header = template

        loop_split = template.split("%%ENUM_LOOP%%")

        if len(loop_split) < 3:
            header = template.replace("%%ENUM_LOOP%%", "")
        else:
            if len(loop_split) % 2 == 0:
                eprint("enum template has an odd number of %%ENUM_LOOP%% markers")
                sys.exit(255)

            header = ""
            for i in range(1, len(loop_split) - 1, 2):
                loop_part = ""
                for e in prop["enum"]:
                    loop_part += loop_split[i].replace("$$ENUM_VALUE$$", e)

                header += loop_split[i - 1] + loop_part

            header += loop_split[-1]

            header = header.replace("$$ENUM_NAME$$", prop["name"])
            header = header.replace("$$BASE_NAME$$", self.msg["name"])

        return header

    def property_pointers(self, prop):
        return "&m%s" %(prop["name"])

    def assignment(self, prop):
        return "m%s = c.m%s;" %(prop["name"], prop["name"])

    def property_definitions(self, prop):
        t = prop["type"]
        if t == 'internal_buffer':
            definition = "InternalBufferProperty(\"%s\", %s)" %(prop["name"], prop["size"]["name"])
        elif t == 'external_buffer':
            definition = "ExternalBufferProperty(\"%s\", %s)" %(prop["name"], prop["size"]["name"])
        elif t == 'const_length_buffer':
            definition = "ConstLengthBufferProperty(\"%s\", %s)" %(prop["name"], prop["size"]["name"])
        elif t == 'string':
            definition = "StringProperty(\"%s\", %s)" %(prop["name"], prop["size"]["name"])
        elif t == 'serializeable':
            definition = "SerializeableProperty(\"%s\", %s)" %(prop["name"], prop["reference"])
        elif t == 'string_set':
            definition = "StringSetProperty(\"%s\", %s, %s)" %(prop["name"], prop["item_length"]["name"], prop["item_number"]["name"])
        elif t == 'length_encoded_set':
            definition = "LengthEncodedSetProperty(\"%s\", %s, %s)" %(prop["name"], prop["item_length"]["name"], prop["item_number"]["name"])
        elif t == 'primitive_set' and prop["item_type"] == 'enum':
            enumName = prop["global"] if "global" in prop else prop["name"] + "_t"
            definition = "PrimitiveSetProperty(\"%s\", %s, %s)" %(prop["name"], enumName, prop["item_number"]["name"])
        elif t == 'primitive_set':
            definition = "PrimitiveSetProperty(\"%s\", %s, %s)" %(prop["name"], prop["item_type"], prop["item_number"]["name"])
        elif t == 'serializeable_set':
            definition = "SerializeableSetProperty(\"%s\", %s, %s)" %(prop["name"], prop["reference"], prop["item_number"]["name"])
        elif t == 'enum_set':
            enumName = prop["global"] if "global" in prop else prop["name"]
            definition = "EnumSetProperty(\"%s\", %sWrapper, %s)" %(prop["name"], enumName, prop["item_number"]["name"])
        elif t == 'uint256':
            definition = "UInt256Property(\"%s\")" %(prop["name"])
        elif t == 'enum':
            enumName = prop["global"] if "global" in prop else prop["name"]
            definition = "EnumProperty(\"%s\", %sWrapper)" %(prop["name"], enumName)
        else:
            definition = "PrimitiveProperty(\"%s\", %s)" %(prop["name"], prop["type"])

        return "decltype (%s) m%s = %s;\n" %(definition, prop["name"], definition)
