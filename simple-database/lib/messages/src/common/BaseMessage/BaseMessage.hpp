
#ifndef __BASEMESSAGE_H__
#define __BASEMESSAGE_H__

#ifndef CRYPTOCURRENCY
#define CRYPTOCURRENCY 0
#endif
#ifndef CONFIDENTIAL_COMPUTE
#define CONFIDENTIAL_COMPUTE 0
#endif

#include "BaseMessageDefinitions.hpp"
#include "Serializeable.hpp"
#include "crc32.h"

using namespace base_message;

class BaseMessage
{
public:

    enum class MessageError {
        None,
        SynchWord,
        UnknownType,
        CRC,
        DataLengthOverflow,
        PartIndexTooLarge,
        NoBuffer,
    };

    BaseMessage();
#ifdef INTERNAL_MSG_BUFFERS
    BaseMessage(uint8_t* buffer, uint32_t len);
#else
    BaseMessage(uint8_t* buffer);
#endif
    virtual ~BaseMessage();


    void Reset();
    void ResetLastError();

    void SerializeMessage();
    crc32_t CalculateCRC();
    void DeserializeMessage();
    void DeserializeMessageHeader();
    virtual uint32_t SerializeBody();
    virtual void DeserializeBody();

    void CheckMessageHeader();
    void CheckCRC();
    void CopyHeader(BaseMessage& source);
    void CopySerializedMessage(BaseMessage& source);

    bool MessageIsResponse();
    bool MessageIsRequest();

    void Origin(BaseMessage& origin);
    void Origin(const MessageHeader& originHeader);

    bool IsInReplyTo(BaseMessage& origin);
    bool IsInReplyTo(crc32_t crc);

    MessageHeader& Header();
    uint8_t*       Buffer();
    uint8_t*       BodyBuffer();
#ifdef INTERNAL_MSG_BUFFERS
    void Buffer(uint8_t* serData, uint32_t dataLength);
#else
    void Buffer(uint8_t* serData);
#endif
    uint32_t     LengthBytes();
    MessageError LastError();
    bool HasError();

    BaseMessage(const BaseMessage&) = default;
    BaseMessage& operator=(const BaseMessage&) = default;

protected:
    void SerializeForCRC();
    void WriteBytes(const void* bytes, uint32_t length);
    void ReadBytes(void* bytes, uint32_t length);
    virtual void Init() {};
    virtual void ResetBody() {};
    void ResetHeader();

    MessageHeader mMsgHeader;
#ifdef INTERNAL_MSG_BUFFERS
    uint8_t       mSerializedMessage[sizeof(struct MessageHeader) + DataMaxLengthBytes];
#else
    uint8_t*      mSerializedMessage = nullptr;
#endif
    MessageError  mLastError = MessageError::None;
    uint32_t      mPosition = 0;

}; //BaseMessage

#endif //__BASEMESSAGE_H__
