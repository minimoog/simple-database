import os, re, sys
from statement_builder import builder

def insert_properties(f, msg, constants):
    primitive_types = ["bool", "uint8_t", "uint16_t", "uint32_t", "uint64_t"]
    getters_setters = []
    getters_setters_header = []
    const_definitions = []
    property_definitions = []
    property_pointers = []
    assignment = []

    statement_builder = builder(msg, constants)

    try:
        props = msg["properties"]
    except KeyError:
        props = []

    templates = os.path.join(os.path.dirname(__file__), "templates/properties")

    primitive_get_set = open(os.path.join(templates, "primitive_get_and_set.txt")).read()
    primitive_get_set_h = open(os.path.join(templates, "primitive_get_and_set_header.txt")).read()
    get = open(os.path.join(templates, "get.txt")).read()
    get_h = open(os.path.join(templates, "get_header.txt")).read()
    set_h = open(os.path.join(templates, "set_header.txt")).read()

    enum_h = open(os.path.join(templates, "enum_header.txt")).read()
    enum = open(os.path.join(templates, "enum.txt")).read()

    for idx, prop in enumerate(props):
        # id means uint64_t
        if prop["type"] == "id":
            prop["type"] = "uint64_t"

        if prop["type"] in primitive_types:
            getters_setters += [statement_builder.get_set(prop, primitive_get_set)]
            getters_setters_header += [statement_builder.get_set_header(prop, primitive_get_set_h)]
        else:
            getters_setters += [statement_builder.get_set(prop, get)]
            getters_setters_header += [statement_builder.get_set_header(prop, get_h)]
            if prop["type"] not in ["serializeable", "primitive_set", "length_encoded_set", "string_set", "serializeable_set", "enum_set"]:
                getters_setters_header += [statement_builder.get_set_header(prop, set_h)]

        if "enum" in prop:
            const_definitions += [statement_builder.enum(prop, enum_h)]
            getters_setters += [statement_builder.enum(prop, enum)]

        const_definitions += [statement_builder.const_definitions(prop)]
        property_definitions += [statement_builder.property_definitions(prop)]
        property_pointers += [statement_builder.property_pointers(prop)]
        assignment += [statement_builder.assignment(prop)]


    f = insert_statements(f, "$$GETTERS_SETTERS$$", getters_setters)
    f = insert_statements(f, "$$GETTERS_SETTERS_HEADER$$", getters_setters_header)
    f = insert_statements(f, "$$CONST_DEFINITIONS$$", const_definitions)
    f = insert_statements(f, "$$PROPERTY_DEFINITIONS$$", property_definitions)

    f["text"] = f["text"].replace("$$PROPERTY_POINTERS$$", ",".join(property_pointers))
    f["text"] = f["text"].replace("$$MAX_SERIALIZED_LENGTH$$", str(msg["max_size"]))
    f["text"] = f["text"].replace("$$NUMBER_OF_PROPERTIES$$", str(len(props)))

    f["text"] = f["text"].replace("$$ASSIGNMENT$$", "\n".join(assignment))

    return f

def insert_statements(f, placeholder, statements):
    statements = trim_statements(statements)
    num_spaces = 0
    p = re.compile("\s*" + placeholder.replace("$", "\$"))
    m = p.search(f["text"])
    if m:
        line = f["text"][m.span()[0]:m.span()[1]].replace("\n", "")
        num_spaces = len(line) - len(line.lstrip())

    indented_statements = []
    for s in statements:
        lines = s.split("\n")
        indented = ""
        for line in lines:
            indented += (" " * num_spaces) + line + "\n"

        indented_statements += [indented]
    f["text"] = f["text"].replace((num_spaces * " ") + placeholder, "\n".join(indented_statements))
    return f

def trim_statements(s):
    s = [st for st in s if st not in [None]]
    trimmed = []
    for st in s:
        st = st.strip(" ")
        st = st.strip("\n")
        trimmed += [st]
    return trimmed
