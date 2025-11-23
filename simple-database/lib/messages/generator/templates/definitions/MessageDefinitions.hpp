#ifndef _MESSAGE_DEFINITIONS_HPP_
#define _MESSAGE_DEFINITIONS_HPP_

#include <stdint.h>
#include <string.h>
#include "FixedLengthString.hpp"

#define HUB_MESSAGE_VERSION {$$VERSION_HASH_BYTE_ARRAY$$}

namespace base_message {
    const uint32_t VersionLength = 16;
    static const uint8_t MessageVersion[VersionLength] = HUB_MESSAGE_VERSION;

    %%CONSTANT_LOOP%%
    static const uint32_t $$CONSTANT_NAME$$ = $$CONSTANT_VALUE$$;
    %%CONSTANT_LOOP%%

    enum class MessageType : uint32_t
    {
        UnknownType = 0,
        $$EVENT_TYPES$$

        UnknownEvent = 999,

        $$TABLE_TYPES$$

        UnknownTable = 1999,

        $$MESSAGE_TYPES$$

        UnknownMessage = 2999,

        $$SERIALIZEABLE_TYPES$$

        UnknownSerializeable = 3999,
    };

    inline FixedLengthString<EnumMaxLength> MessageTypeToString(MessageType m)
    {
        switch(m) {
            %%TYPE_LOOP%%
            case MessageType::$$MESSAGE_TYPE$$Type:
                return "$$MESSAGE_TYPE$$";
            %%TYPE_LOOP%%
            default:
                return "";
        }
    }

    inline MessageType MessageTypeFromString(FixedLengthString<EnumMaxLength> str)
    {
        %%TYPE_LOOP%%
        if (str == "$$MESSAGE_TYPE$$") {
            return MessageType::$$MESSAGE_TYPE$$Type;
        }
        %%TYPE_LOOP%%

        return (MessageType)0;
    }
};

#endif
