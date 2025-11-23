# Message Generator
This is the message generator. It is a python 3 program that generates message classes, for use in other projects.
Currently C++, C# and JavaScript are supported

## Installation
There are no dependecies outside of the standard python 3 libraries.

## Usage

```python3 ./message_generator.py```

The generated messages will go into the output/ folder (This contains submodules for each supported language).

The messages to generate should be defined in the files *json/Misc.json*, *json/Requests.json*, *json/Responses.json*.
Each of these files should contain an array of Message objects. 
Every object is required to have a "name", 
which is used to generate the class name and MessageType enum.
Additionally, a message can have an array of "properties".


### properties
Each property is required to also have a "name" attribute, as well as a "type" corresponding to one of the *argument_types* defined in *message_definitions.py*.

#### string, internal_buffer, external_buffer, const_length_buffer
If the property type is an array or string, a "size" attribute is required.
The size attribute should contain a "name" and a "value" attribute.
The name will become a constant definition assigned to the value, 
and this will define the maximum (or constant) size of the array or string.
If the name matches a definition in the constants defined in `message_definitions.py`, that value will be referenced instead
The value may be ommitted if th name appears in the message constants defined in `message_definitons.py`

#### string_set, length_encoded_set
If the property type is a set it is required to have the attributes "item", "item_number", and "item_length".

The "item" attribute should be a string that defines the name of the item that goes into a set.
E.g, for the property "Addresses" the item might be "Address".

The "item_number" attribute defines the maximum number of items that can be in the set, 
and has the same structure as the "size" attributes used for arrays.

The "item_length" attribute defines the maximum length of a single item, 
and has the same structure as the "size" attributes used for arrays.


### Notes:

* Avoid using keywords for any message or property names (such as *Byte*, since *byte* is reserved in microsoft-java).

* All "name" attributes should be TitleCase. This is encforced by *json_parser.py*.

* The *BaseMessage* (and *ResponseMessage*) classes are assumed to exist for each language, as this cannot be generated. Only the subclasses can be generated.

## How does it work?

* When the message_generator.py script is run, it loads the json and parses it using the *json_parser.py* script.

* Once the messages have been parsed the generator will loop through each language and create the message classes based on the templates in the top level of templates/$LANGUAGE/

* Templates should contain markers that indicate where things need to be inserted in the class, such as $$CLASS_NAME$$ or $$SERIALIZE$$.

* For each message the template will be filled in using the *insert_properties* function, defined in *property_generator.py*. 
The property generator will create a *statement_builder* for the current language, and use this to replace each of the tokens defined in the template.

* The statement builders all subclass *base_builder.py*, and each new language will require a new statement builder to be inplemented.

* Once all of the property specific information has been inserted, the file will be saved to the output/$LANGUAGE/ folder corresponding to the current language.
