
#include "Receiver.hpp"

Receiver::Receiver()
{
}

Receiver::~Receiver()
{
}

Receiver::Receiver(BaseMessage *message)
{
    Initialize(message);
}

void Receiver::Initialize(BaseMessage *message)
{
    mRxMessage = message;
}

bool Receiver::HandleBytes(const uint8_t* data, uint32_t len)
{
    uint32_t i = 0;
    for (i = 0; i < len; i++) {
        if (HandleByte(*(data + i))) {
            return true;
        }
    }
    return false;
}

bool Receiver::HandleByte(uint8_t b)
{
    switch(mState.state) {
        case ReceptionState::SyncSearch:
            mSyncWord >>= 8;
            mSyncWord |= (b << 24);
            if (mSyncWord == base_message::MessageSynchWord) {
                memcpy(mRxMessage->Buffer(), &mSyncWord, sizeof(mSyncWord));
                mState.MoveToNextState();
            }
            break;
        case ReceptionState::LengthReception:
            *(mRxMessage->Buffer() + mState.bytesReceived++) = b;
            if (mState.bytesReceived == mState.expectedBytes) {
                mState.expectedBytes = *(uint32_t*)(mRxMessage->Buffer() + 4);
                mState.MoveToNextState();
                // if the length is longer than our max its probably messed up and we def can't read it
                if (mState.expectedBytes > base_message::MessageLength) {
                    mState.Reset();
                    break;
                }
            }
            break;
        case ReceptionState::HeaderReception:
            *(mRxMessage->Buffer() + mState.bytesReceived++) = b;
            if (mState.bytesReceived == mState.expectedBytes) {
                mRxMessage->DeserializeMessageHeader();
                if (mRxMessage->HasError()) {
                    mState.Reset();
                    mRxMessage->ResetLastError();
                    break;
                }
                if (mRxMessage->Header().bodyLength == 0) {
                    mRxMessage->CheckCRC();
                    if (mRxMessage->HasError()) {
                        mState.Reset();
                        mRxMessage->ResetLastError();
                        break;
                    }
                    mState.Reset();
                    return true;
                }
                mState.expectedBytes = mState.bytesReceived + mRxMessage->Header().bodyLength;
                mState.MoveToNextState();
            }
            break;
        case ReceptionState::DataReception:
            *(mRxMessage->Buffer() + mState.bytesReceived++) = b;
            if (mState.bytesReceived == mState.expectedBytes) {
                mRxMessage->CheckCRC();
                if (mRxMessage->HasError()) {
                    mState.Reset();
                    mRxMessage->ResetLastError();
                    break;
                }
                mState.Reset();
                return true;
            }
            break;
        default: break;
    }

    return false;
}

uint32_t Receiver::CurrentReadLength()
{
    if (mState.state == ReceptionState::SyncSearch) {
        return 1;
    }
    return mState.expectedBytes - mState.bytesReceived;
}
