

#include "BaseMessage.hpp"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

BaseMessage::BaseMessage()
{
}

#ifdef INTERNAL_MSG_BUFFERS
BaseMessage::BaseMessage(uint8_t* buffer, uint32_t len)
{
    Buffer(buffer, len);
}
#else
BaseMessage::BaseMessage(uint8_t* buffer)
{
    Buffer(buffer);
}
#endif

BaseMessage::~BaseMessage()
{
}

void BaseMessage::Reset()
{
    ResetHeader();
    ResetBody();
    Init();
}

void BaseMessage::ResetHeader()
{
    mMsgHeader = MessageHeader();
}

void BaseMessage::ResetLastError()
{
    mLastError = MessageError::None;
}


void BaseMessage::SerializeMessage()
{
    if (mSerializedMessage == nullptr)
    {
        mLastError = MessageError::NoBuffer;
        return;
    }

    SerializeForCRC();
    if (mLastError != MessageError::None)
    {
        return;
    }

    mMsgHeader.crc = CalculateCRC();
    *(crc32_t*)(mSerializedMessage + MessageCrcIndex) = mMsgHeader.crc;
}

crc32_t BaseMessage::CalculateCRC()
{
    *(crc32_t*)(mSerializedMessage + MessageCrcIndex) = 0;
    return crc32(mSerializedMessage, mMsgHeader.headerLength + mMsgHeader.bodyLength);
}

void BaseMessage::DeserializeMessage()
{
    if (mSerializedMessage == nullptr)
    {
        mLastError = MessageError::NoBuffer;
        return;
    }

    DeserializeMessageHeader();

    if (mLastError != MessageError::None)
    {
        return;
    }

    if (mMsgHeader.bodyLength > 0)
    {
        DeserializeBody();
    }

    CheckCRC();
}

void BaseMessage::DeserializeMessageHeader()
{
    if (mSerializedMessage == nullptr)
    {
        mLastError = MessageError::NoBuffer;
        return;
    }

    mPosition = 0;

    ReadBytes(&mMsgHeader.synchWord, sizeof(mMsgHeader.synchWord));
    ReadBytes(&mMsgHeader.headerLength, sizeof(mMsgHeader.headerLength));
    ReadBytes(&mMsgHeader.bodyLength, sizeof(mMsgHeader.bodyLength));
    ReadBytes(&mMsgHeader.counter, sizeof(mMsgHeader.counter));
    ReadBytes(&mMsgHeader.timeStamp, sizeof(mMsgHeader.timeStamp));
    ReadBytes(mMsgHeader.version, VersionLength);
    ReadBytes(&mMsgHeader.totalParts, sizeof(mMsgHeader.totalParts));
    ReadBytes(&mMsgHeader.currentPartIndex, sizeof(mMsgHeader.currentPartIndex));
    ReadBytes(&mMsgHeader.crc, sizeof(mMsgHeader.crc));
    ReadBytes(&mMsgHeader.origin, sizeof(mMsgHeader.origin));

    auto typeStr = EnumString();
    mPosition += typeStr.Deserialize(mSerializedMessage + mPosition);
    mMsgHeader.typeOfMessage = MessageTypeFromString(typeStr);

    if (strnlen((char*)mSerializedMessage + mPosition, TokenMaxLength + 1) > TokenMaxLength) {
        mLastError = MessageError::DataLengthOverflow;
        return;
    }
    strcpy(mMsgHeader.userToken, (char*)mSerializedMessage + mPosition);
    mPosition += strlen(mMsgHeader.userToken) + 1;

    ReadBytes(&mMsgHeader.actionId, sizeof(mMsgHeader.actionId));

    if (strnlen((char*)mSerializedMessage + mPosition, MaxRequestIdLength + 1) > MaxRequestIdLength) {
        mLastError = MessageError::DataLengthOverflow;
        return;
    }
    strcpy(mMsgHeader.requestId, (char*)mSerializedMessage + mPosition);
    mPosition += strlen(mMsgHeader.requestId) + 1;
    ReadBytes(&mMsgHeader.actionExpirationTimeInMinutes, sizeof(mMsgHeader.actionExpirationTimeInMinutes));

    // if we've read past what the header says is its length
    // something must've gone wrong
    if (mPosition > mMsgHeader.headerLength) {
        mLastError = MessageError::DataLengthOverflow;
        return;
    }

    CheckMessageHeader();
}

void BaseMessage::SerializeForCRC()
{
    if (mSerializedMessage == nullptr)
    {
        mLastError = MessageError::NoBuffer;
        return;
    }

    CheckMessageHeader();
    if (mLastError != MessageError::None)
    {
        return;
    }

    mPosition = mMsgHeader.setHeaderLength();
    uint32_t bodyLength = SerializeBody();

    if (bodyLength > 0) { //as to not overwrite the header when we serialize as the base class
        mMsgHeader.bodyLength = bodyLength;
    }

    mPosition = 0;
    mMsgHeader.crc = 0;

    WriteBytes(&mMsgHeader.synchWord, sizeof(mMsgHeader.synchWord));
    WriteBytes(&mMsgHeader.headerLength, sizeof(mMsgHeader.headerLength));
    WriteBytes(&mMsgHeader.bodyLength, sizeof(mMsgHeader.bodyLength));
    WriteBytes(&mMsgHeader.counter, sizeof(mMsgHeader.counter));
    WriteBytes(&mMsgHeader.timeStamp, sizeof(mMsgHeader.timeStamp));
    WriteBytes(mMsgHeader.version, VersionLength);
    WriteBytes(&mMsgHeader.totalParts, sizeof(mMsgHeader.totalParts));
    WriteBytes(&mMsgHeader.currentPartIndex, sizeof(mMsgHeader.currentPartIndex));
    WriteBytes(&mMsgHeader.crc, sizeof(mMsgHeader.crc));
    WriteBytes(&mMsgHeader.origin, sizeof(mMsgHeader.origin));
    auto str = MessageTypeToString(mMsgHeader.typeOfMessage);
    WriteBytes((const char*)str, MIN(EnumMaxLength, strlen(str)) + 1);
    WriteBytes(mMsgHeader.userToken, MIN(TokenMaxLength, strlen(mMsgHeader.userToken)) + 1);
    WriteBytes(&mMsgHeader.actionId, sizeof(mMsgHeader.actionId));
    WriteBytes(mMsgHeader.requestId, MIN(MaxRequestIdLength, strlen(mMsgHeader.requestId)) + 1);
    WriteBytes(&mMsgHeader.actionExpirationTimeInMinutes, sizeof(mMsgHeader.actionExpirationTimeInMinutes));
}

void BaseMessage::CheckMessageHeader()
{
    mLastError = MessageError::None;

    if (mMsgHeader.synchWord != MessageSynchWord)
    {
        mLastError = MessageError::SynchWord;
        return;
    }

    if (mMsgHeader.currentPartIndex > mMsgHeader.totalParts - 1)
    {
        mLastError = MessageError::PartIndexTooLarge;
        return;
    }

    if (strlen(mMsgHeader.userToken) > TokenMaxLength)
    {
        mLastError = MessageError::DataLengthOverflow;
        return;
    }

    if (strlen(mMsgHeader.requestId) > MaxRequestIdLength)
    {
        mLastError = MessageError::DataLengthOverflow;
        return;
    }

    if (mMsgHeader.bodyLength > MessageLength - mMsgHeader.headerLength)
    {
        mLastError = MessageError::DataLengthOverflow;
        return;
    }

    if (mMsgHeader.typeOfMessage == MessageType::UnknownType ||
        mMsgHeader.typeOfMessage == MessageType::UnknownEvent ||
        mMsgHeader.typeOfMessage == MessageType::UnknownTable ||
        mMsgHeader.typeOfMessage == MessageType::UnknownMessage ||
        mMsgHeader.typeOfMessage == MessageType::UnknownSerializeable)
    {
        mLastError = MessageError::UnknownType;
        return;
    }
}

void BaseMessage::CheckCRC()
{
    if (mSerializedMessage == nullptr)
    {
        mLastError = MessageError::NoBuffer;
        return;
    }

    crc32_t refCRC = CalculateCRC();

    if (refCRC != mMsgHeader.crc) {
        mLastError = MessageError::CRC;
        return;
    }

    // Put it back
    *(crc32_t*)(mSerializedMessage + MessageCrcIndex) = mMsgHeader.crc;
}

void BaseMessage::CopyHeader(BaseMessage& source)
{
    mMsgHeader = source.Header();
}

void BaseMessage::CopySerializedMessage(BaseMessage& source)
{
    memcpy(mSerializedMessage, source.Buffer(), source.LengthBytes());
}

uint32_t BaseMessage::SerializeBody()
{
    return 0;
}

void BaseMessage::DeserializeBody()
{
}

void BaseMessage::WriteBytes(const void* bytes, uint32_t length)
{
    memcpy(mSerializedMessage + mPosition, bytes, length);
    mPosition += length;
}

void BaseMessage::ReadBytes(void* bytes, uint32_t length)
{
    memcpy(bytes, mSerializedMessage + mPosition, length);
    mPosition += length;
}

MessageHeader& BaseMessage::Header()
{
    return mMsgHeader;
}

uint8_t* BaseMessage::Buffer()
{
    return mSerializedMessage;

}

#ifdef INTERNAL_MSG_BUFFERS
void BaseMessage::Buffer(uint8_t* serData, uint32_t dataLength)
{
    memcpy(mSerializedMessage, serData, dataLength);
}
#else
void BaseMessage::Buffer(uint8_t* serData)
{
    mSerializedMessage = serData;
}
#endif


uint32_t BaseMessage::LengthBytes()
{
    return mMsgHeader.bodyLength + mMsgHeader.headerLength;
}

BaseMessage::MessageError BaseMessage::LastError()
{
    return mLastError;
}

bool BaseMessage::HasError()
{
    return mLastError != MessageError::None;
}

bool BaseMessage::MessageIsResponse()
{
    return (uint32_t)mMsgHeader.typeOfMessage >= 1000 &&
        (uint32_t)mMsgHeader.typeOfMessage < 2000;
}

bool BaseMessage::MessageIsRequest()
{
    return (uint32_t)mMsgHeader.typeOfMessage >= 2000 &&
        (uint32_t)mMsgHeader.typeOfMessage < 3000;
}

void BaseMessage::Origin(BaseMessage& origin)
{
    Origin(origin.Header());
}

void BaseMessage::Origin(const MessageHeader& originHeader)
{
    mMsgHeader.origin = originHeader.crc;
}

bool BaseMessage::IsInReplyTo(BaseMessage& origin)
{
    return IsInReplyTo(origin.Header().crc);
}

bool BaseMessage::IsInReplyTo(crc32_t crc)
{
    return crc == mMsgHeader.origin;
}

uint8_t* BaseMessage::BodyBuffer()
{
    return Buffer() + Header().headerLength;
}
