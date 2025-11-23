#include <gtest/gtest.h>
#include "Receiver.hpp"
#include "BaseMessageDefinitions.hpp"
#include "KeepAliveMessage.hpp"
#include "TestReqMessage.hpp"

class ReceiverTest : public ::testing::Test {
    protected:
        virtual void SetUp() override {
            memset(msgBuffer, 0, sizeof(msgBuffer));
        }

        virtual void TearDown() override {
        }

        uint8_t msgBuffer[base_message::MessageLength];
};

TEST_F(ReceiverTest, ReceiverStartsInCorrectState) {
    BaseMessage msg{msgBuffer};
    Receiver r{&msg};
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0xd5);
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0x55);
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0x55);
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0x55);
    EXPECT_EQ(4, r.CurrentReadLength());
}

TEST_F(ReceiverTest, ReceiverHandlesGarbageData) {
    BaseMessage msg{msgBuffer};
    Receiver r{&msg};
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0xd5);
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0xd5);
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0xd5);
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0x55);
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0x55);
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleByte(0x55);
    EXPECT_EQ(4, r.CurrentReadLength());
}

TEST_F(ReceiverTest, ReceiverParsesLengthToBeRead) {
    BaseMessage msg{msgBuffer};
    Receiver r{&msg};
    EXPECT_EQ(1, r.CurrentReadLength());
    r.HandleBytes((uint8_t*)&base_message::MessageSynchWord, sizeof(base_message::MessageSynchWord));
    EXPECT_EQ(4, r.CurrentReadLength());
    uint32_t headerLen = 80;
    r.HandleBytes((uint8_t*)&headerLen, 4);
    EXPECT_EQ(headerLen - 8, r.CurrentReadLength());
}

TEST_F(ReceiverTest, ReceiverCanReceiveAMessageWithoutBody) {
    BaseMessage msg{msgBuffer};
    Receiver r{&msg};
    uint8_t buff[base_message::MessageLength];
    KeepAliveMessage keep{buff};
    keep.SerializeMessage();

    EXPECT_TRUE(r.HandleBytes(buff, keep.LengthBytes()));
    msg.DeserializeMessage();
    EXPECT_EQ(msg.Header().typeOfMessage, base_message::MessageType::KeepAliveType);
}

TEST_F(ReceiverTest, ReceiverCanReceiveAMessageWithBody) {
    BaseMessage msg{msgBuffer};
    Receiver r{&msg};
    uint8_t buff[base_message::MessageLength];
    uint8_t externalBuff[TestReq::MaxExternalBuffSize] = {0};
    TestReqMessage test{buff};
    test.Body().ExternalBuff() = externalBuff;
    test.SerializeMessage();

    EXPECT_TRUE(r.HandleBytes(buff, test.LengthBytes()));

    msg.DeserializeMessage();
    EXPECT_EQ(msg.Header().typeOfMessage, base_message::MessageType::TestReqType);
    EXPECT_EQ(msg.Header().bodyLength, test.Header().bodyLength);
}

TEST_F(ReceiverTest, ReceiverChecksCrcOnMessageWithoutBody ) {
    BaseMessage msg{msgBuffer};
    Receiver r{&msg};
    uint8_t buff[base_message::MessageLength];
    KeepAliveMessage test{buff};
    test.SerializeMessage();
    (*(test.Buffer() + base_message::MessageCrcIndex))++;

    EXPECT_FALSE(r.HandleBytes(buff, test.LengthBytes()));
}
