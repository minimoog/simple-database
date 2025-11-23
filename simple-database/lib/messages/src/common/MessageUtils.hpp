#ifndef _MESSAGEUTILS_HPP_
#define _MESSAGEUTILS_HPP_

#include "BaseMessage.hpp"
#include "MessageEnums.hpp"

namespace base_message {
    ErrorCode MessageErrorToCode(BaseMessage::MessageError err);
};

#endif //_MESSAGEUTILS_HPP_
