
#ifndef BASEMESSAGEDEFINITIONS_H_
#define BASEMESSAGEDEFINITIONS_H_

#include <cstdint>
#include "MessageDefinitions.hpp"
#include "crc32.h"
#include <climits>

namespace base_message
{
    const uint32_t MessageSynchWord = 0x555555d5;

    const uint32_t BodyMaxLength = MessageLength - HeaderMinLength;

    struct MessageHeader
    {
        uint32_t synchWord = MessageSynchWord;
        uint32_t headerLength = HeaderMinLength;
        uint32_t bodyLength = 0;
        uint32_t counter = 0;
        uint64_t timeStamp = 0;
        uint8_t version[VersionLength] = HUB_MESSAGE_VERSION;
        uint32_t totalParts = 1;
        uint32_t currentPartIndex = 0;
        crc32_t crc = 0;
        crc32_t origin = 0;
        MessageType typeOfMessage = MessageType::UnknownType;
        char userToken[TokenMaxLength + 1] = {0};
        uint64_t actionId = 0;
        char requestId[MaxRequestIdLength + 1] = {0};
        uint64_t actionExpirationTimeInMinutes = 0;

        uint32_t setHeaderLength() {
            headerLength = HeaderMinLength + strlen(userToken) + strlen(MessageTypeToString(typeOfMessage)) + strlen(requestId);
            return headerLength;
        }
    };

    const uint32_t MessageCrcIndex = 48;
};



#endif /* BASEMESSAGEDEFINITIONS_H_ */
