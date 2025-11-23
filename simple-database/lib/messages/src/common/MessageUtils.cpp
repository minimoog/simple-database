#include "MessageUtils.hpp"

using namespace base_message;

ErrorCode base_message::MessageErrorToCode(BaseMessage::MessageError err)
{
        switch(err) {
            case BaseMessage::MessageError::None:
                return ErrorCode::None;
            case BaseMessage::MessageError::UnknownType:
                return ErrorCode::MessageUnknownMessageType;
            case BaseMessage::MessageError::CRC:
                return ErrorCode::MessageCRC;
            case BaseMessage::MessageError::DataLengthOverflow:
                return ErrorCode::MessageDataLengthOverflow;
            case BaseMessage::MessageError::PartIndexTooLarge:
                return ErrorCode::MessagePartIndexTooLarge;
            default:
                return ErrorCode::General;
        }
    }
