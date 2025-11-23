#ifndef _$$BASE_NAME_UPPER$$_H_
#define _$$BASE_NAME_UPPER$$_H_
#if $$IFDEF$$

#include "Serializeable.hpp"
#include "BaseMessage.hpp"
#include "Property.hpp"
#include "MessageDefinitions.hpp"
#include "MessageEnums.hpp"
#include <functional>

$$INCLUDED_SERIALIZEABLES$$

using namespace base_message;

class $$BASE_NAME$$ : public Serializeable
{
public:
    static constexpr uint32_t MaxSerializedLength = $$MAX_SERIALIZED_LENGTH$$;
    static_assert(MaxSerializedLength <= BodyMaxLength);
    static const char * SerializeableName;
    static constexpr MessageType SerializeableType = MessageType::$$MESSAGE_TYPE$$;

    $$CONST_DEFINITIONS$$

private:
    $$PROPERTY_DEFINITIONS$$

    BaseProperty* properties[$$NUMBER_OF_PROPERTIES$$] = {
        $$PROPERTY_POINTERS$$
    };

public:
    $$BASE_NAME$$() = default;
    virtual ~$$BASE_NAME$$() = default;
    $$BASE_NAME$$(const uint8_t* buffer);
    $$BASE_NAME$$(BaseMessage& message);
    $$BASE_NAME$$($$BASE_NAME$$&&c) = default;
    $$BASE_NAME$$& operator=($$BASE_NAME$$&&c) = default;

    uint32_t NumberOfProperties() const override { return $$NUMBER_OF_PROPERTIES$$; };
    const BaseProperty * const* Properties() const override { return &properties[0]; };


    $$GETTERS_SETTERS_HEADER$$

    $$BASE_NAME$$( const $$BASE_NAME$$ &c ) : Serializeable(c) {
        *this = c;
    }
    $$BASE_NAME$$& operator=( const $$BASE_NAME$$ &c ) {
        if (this == &c) return *this;
        $$ASSIGNMENT$$
        return *this;
    }

private:
    $$SERIALIZEABLE_FRIEND_CLASSES$$

    template<typename T> friend class Property;
    template<typename T, size_t size> friend struct std::array;
    template <typename T, size_t size> friend class LengthPrefixedSet;
};

#endif
#endif
