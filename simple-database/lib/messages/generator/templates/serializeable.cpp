#if $$IFDEF$$
#include "$$BASE_NAME$$.hpp"

static const char _NameData[] = {"$$BASE_NAME$$"};
const char* $$BASE_NAME$$::SerializeableName = _NameData;

$$BASE_NAME$$::$$BASE_NAME$$(const uint8_t* buffer)
{
    Serializeable::Deserialize(buffer);
}

$$BASE_NAME$$::$$BASE_NAME$$(BaseMessage& message)
{
    Serializeable::Deserialize(message.Buffer() + message.Header().headerLength);
}

$$GETTERS_SETTERS$$
#endif
