#if $$IFDEF$$
#include "$$CLASS_NAME$$.hpp"

$$CLASS_NAME$$::$$CLASS_NAME$$()
{
    $$CLASS_NAME$$::Init();
}

$$CLASS_NAME$$::$$CLASS_NAME$$(uint8_t* buffer)
{
    $$CLASS_NAME$$::Init();
    Buffer(buffer);
}

$$CLASS_NAME$$::$$CLASS_NAME$$(BaseMessage& message)
{
    $$CLASS_NAME$$::Init();
    Buffer(message.Buffer());
    DeserializeMessage();
}
$$CLASS_NAME$$::$$CLASS_NAME$$($$BASE_NAME$$& body) : mBody{body}
{
    $$CLASS_NAME$$::Init();
}

$$CLASS_NAME$$::~$$CLASS_NAME$$()
{
}

void $$CLASS_NAME$$::Init()
{
    mMsgHeader.typeOfMessage = MessageType::$$MESSAGE_TYPE$$;
}

void $$CLASS_NAME$$::ResetBody()
{
    mBody.Reset();
}

uint32_t $$CLASS_NAME$$::SerializeBody()
{
    mPosition = mMsgHeader.headerLength;
    return mPosition + mBody.Serialize(mSerializedMessage + mPosition) - mMsgHeader.headerLength;
}

void $$CLASS_NAME$$::DeserializeBody()
{
    mPosition = mMsgHeader.headerLength;
    mBody.Deserialize(mSerializedMessage + mPosition);
}

$$BASE_NAME$$& $$CLASS_NAME$$::Body()
{
    return mBody;
}
#endif
