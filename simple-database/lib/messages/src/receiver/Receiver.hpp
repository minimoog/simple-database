
#ifndef _RECEIVER_HPP_
#define _RECEIVER_HPP_
#include "BaseMessage.hpp"

class Receiver
{
public:
    Receiver();
    ~Receiver();
    Receiver(BaseMessage *message);

    void Initialize(BaseMessage *message);

    bool HandleBytes(const uint8_t* data, uint32_t len);
    bool HandleByte(uint8_t b);
    uint32_t CurrentReadLength();
private:
    enum class ReceptionState : uint32_t {
        SyncSearch,
        LengthReception,
        HeaderReception,
        DataReception,

        End,
    };

    struct State {
        ReceptionState state;
        uint32_t bytesReceived;
        uint32_t expectedBytes;
        uint32_t headerLength;

        State() {
            Reset();
        }

        void MoveToNextState() {
            state = (ReceptionState)((uint32_t)state + 1);
            if (state == ReceptionState::End) state = ReceptionState::SyncSearch;

            if (state == ReceptionState::SyncSearch) bytesReceived = 0;

            if (state == ReceptionState::LengthReception) {
                bytesReceived = 4;
                expectedBytes = 8;
            }
        }

        void Reset() {
            state = ReceptionState::SyncSearch;
            bytesReceived = 0;
            headerLength = 0;
            expectedBytes = sizeof(base_message::MessageHeader);
        }
    };

    State mState;
    uint32_t mSyncWord = 0;

    BaseMessage *mRxMessage;
    uint8_t mPubKey[33];
    Receiver( const Receiver &c );
    Receiver& operator=( const Receiver &c );
};

#endif //_RECEIVER_HPP_
