#include "Serializeable.hpp"
#include "MessageDefinitions.hpp"
#include "BaseMessage.hpp"

void Serializeable::Reset()
{
    size_t i = 0;
    while (BaseProperty* prop = PropertyAtIndex(i++)) {
        prop->Reset();
    }
}

size_t Serializeable::TotalLength() const {
    size_t sum = 0;
    size_t i = 0;
    while (const BaseProperty* prop = PropertyAtIndex(i++)) {
        sum += prop->TotalLength();
    }

    return sum;
}

uint32_t Serializeable::Serialize(void *buffer, bool compact) const {
    uint32_t pos = 0;

    size_t i = 0;
    while (const BaseProperty* prop = PropertyAtIndex(i++)) {
        uint32_t lastWrite = prop->Serialize((uint8_t*)buffer + pos);
        pos += compact ? lastWrite : prop->MaxLength();
    }

    return pos;
}

uint32_t Serializeable::Deserialize(const void *buffer, bool compact) {
    uint32_t pos = 0;

    size_t i = 0;
    while (BaseProperty* prop = PropertyAtIndex(i++)) {
        uint32_t lastWrite = prop->Deserialize((uint8_t*)buffer + pos);
        pos += compact ? lastWrite : prop->MaxLength();
    }

    return pos;
}

uint32_t Serializeable::MaxLength() const
{
    size_t sum = 0;
    size_t i = 0;
    while (const BaseProperty* prop = PropertyAtIndex(i++)) {
        sum += prop->MaxLength();
    }
    return sum;
}

uint32_t Serializeable::NumberOfProperties() const
{
    return 0;
}

const BaseProperty* const* Serializeable::Properties() const
{
    return nullptr;
}

const BaseProperty* Serializeable::PropertyByName(const char* name) const
{
    size_t i = 0;
    while (const BaseProperty* prop = PropertyAtIndex(i++)) {
        if (0 == strcmp(name, prop->Name())) {
            return prop;
        }
    }

    return nullptr;
};

bool Serializeable::operator==(const Serializeable& rhs) const
{
    uint8_t bufferOne[base_message::BodyMaxLength] = {0};
    uint8_t bufferTwo[base_message::BodyMaxLength] = {0};
    uint32_t rhsLength = rhs.Serialize(bufferTwo);
    if(rhsLength != this->Serialize(bufferOne))
        return false;
    return memcmp(bufferOne, bufferTwo, rhsLength) == 0;
}

bool Serializeable::operator!=(const Serializeable& rhs) const
{
    return !(*(this) == rhs);
}

uint32_t Serializeable::NonCompactPropertyPosition(const char* name) const
{
    size_t i = 0;
    size_t pos = 0;
    while (const BaseProperty* prop = PropertyAtIndex(i++)) {
        if (0 == strcmp(name, prop->Name())) {
            return pos;
        }
        pos += prop->MaxLength();
    }

    assert(false);
    return 0;
};

const char* Serializeable::SerializeableName() const
{
    return "";
}

const BaseProperty* Serializeable::PropertyAtIndex(size_t i) const {
    const BaseProperty* const* props = Properties();

    if (i >= NumberOfProperties() || !props) {
        return nullptr;
    }

    return *(props + i);
}
