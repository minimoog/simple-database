#include <gtest/gtest.h>
#include <vector>
#include <string.h>
#include "BaseMessage.hpp"
#include "TestReqMessage.hpp"
#include "TestNestedMessage.hpp"
#include "TestCompileFlagsCCMessage.hpp"
#include "TestCompileFlagsCryptoMessage.hpp"

class BaseMessageTest : public ::testing::Test {
    protected:
        virtual void SetUp() {
            memset(msgBuffer, 0, sizeof(msgBuffer));
            memset(externalBuff, 0, sizeof(externalBuff));
        }

        virtual void TearDown() {
        }

        void ReCRC(BaseMessage& msg) {
            msg.Header().crc = msg.CalculateCRC();
            *(crc32_t*)(msg.Buffer() + MessageCrcIndex) = msg.Header().crc;
        }

        void SetTestReq(TestReq& req) {
            req.UintEight(0xFF - 1);
            req.UintSixteen(0xFFFF - 1);
            req.UintThirtyTwo(0xFFFFFFFF - 1);
            req.UintSixtyFour(0xFFFFFFFFFFFFFFFF - 1);
            req.Baalean(true);
            req.Strang("yo");
            req.Enam(TestReq::Enam_t::Test);
            req.GlobalEnum(TestEnum::Two);
            req.MessageTypeEnum(MessageType::TestReqType);

            uint8_t internalBuff[TestReq::MaxInternalBuffSize];
            memset(internalBuff, 0xD5, sizeof(internalBuff));
            req.InternalBuff(internalBuff, sizeof(internalBuff));

            memset(externalBuff + 4, 0x55, sizeof(externalBuff) - 4);
            req.ExternalBuff() = {externalBuff, sizeof(externalBuff)};

            uint8_t constLengthBuff[TestReq::ConstLengthBuffLen];
            memset(constLengthBuff, 0xEE, sizeof(constLengthBuff));
            req.ConstLengthBuff(constLengthBuff);

            uint8_t uint256Buff[32];
            memset(uint256Buff, 0xFF, sizeof(uint256Buff));
            req.UIntTwoFiveSix(uint256Buff);

            std::vector<const char*> data {
                "feed",
                "me",
                "see",
                "more",
            };
            for (const auto& d: data) {
                req.ADataSet().PushItem({(uint8_t*)d, (uint32_t)strlen(d)});
            }

            std::vector<const char*> strings {
                "you",
                "can",
                "do",
                "it",
                "!",
            };
            for (const auto& s: strings) {
                req.SomeStrings().PushItem(s);
            }

            req.PrimitiveSetOfUintThirtyTwo().PushItem(1);
            req.PrimitiveSetOfUintThirtyTwo().PushItem(2);

            req.PrimitiveSetOfUintEight().PushItem(2);
            req.PrimitiveSetOfUintEight().PushItem(3);

            req.SetOfEnum().PushItem(TestReq::SetOfEnum_t::Lol);
            req.SetOfEnum().PushItem(TestReq::SetOfEnum_t::Jk);

            req.SetOfGlobalEnum().PushItem(TestEnum::One);
            req.SetOfGlobalEnum().PushItem(TestEnum::Two);
        }
        uint8_t msgBuffer[base_message::MessageLength];
        uint8_t externalBuff[TestReq::MaxExternalBuffSize];
};

TEST_F(BaseMessageTest, headerSizeMatchesDefinedConstant) {
    EXPECT_GE(sizeof(base_message::MessageHeader), base_message::HeaderMinLength);
}

TEST_F(BaseMessageTest, basicSerialization) {
    BaseMessage msg{msgBuffer};
    msg.Header().timeStamp = 200;
    msg.Header().counter = 400;
    msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
    msg.Header().totalParts = 3;
    msg.Header().currentPartIndex = 2;
    strcpy(msg.Header().requestId, "61b9a376-f16d-11ec-8ea0-0242ac120002");
    msg.SerializeMessage();

    EXPECT_FALSE(msg.HasError());
    EXPECT_NE(0, msg.Header().crc);

    BaseMessage outMsg{msgBuffer};
    outMsg.DeserializeMessage();
    EXPECT_EQ(outMsg.LastError(), BaseMessage::MessageError::None);
    EXPECT_EQ(msg.Header().synchWord, outMsg.Header().synchWord);
    EXPECT_EQ(msg.Header().headerLength, outMsg.Header().headerLength);
    EXPECT_EQ(msg.Header().bodyLength, outMsg.Header().bodyLength);
    EXPECT_EQ(msg.Header().counter, outMsg.Header().counter);
    EXPECT_EQ(msg.Header().timeStamp, outMsg.Header().timeStamp);
    EXPECT_TRUE(0 == memcmp(msg.Header().version, outMsg.Header().version, VersionLength));
    EXPECT_EQ(msg.Header().typeOfMessage, outMsg.Header().typeOfMessage);
    EXPECT_EQ(msg.Header().totalParts, outMsg.Header().totalParts);
    EXPECT_EQ(msg.Header().currentPartIndex, outMsg.Header().currentPartIndex);
    EXPECT_EQ(msg.Header().crc, outMsg.Header().crc);
}

TEST_F(BaseMessageTest, SerializeRequestIdDataOverflow) {
    BaseMessage msg{msgBuffer};
    msg.Header().timeStamp = 200;
    msg.Header().counter = 400;
    msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
    msg.Header().totalParts = 3;
    msg.Header().currentPartIndex = 2;
    strcpy(msg.Header().requestId, "61b9a376-f16d-11ec-8ea0-0242ac1200021");
    msg.SerializeMessage();

    EXPECT_EQ(msg.LastError(), BaseMessage::MessageError::DataLengthOverflow);

}
TEST_F(BaseMessageTest, basicSerializationWithAToken) {
    BaseMessage msg{msgBuffer};
    msg.Header().timeStamp = 200;
    msg.Header().counter = 400;
    msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
    msg.Header().totalParts = 3;
    msg.Header().currentPartIndex = 2;
    const char* token = "This is a fantastic token";
    strcpy(msg.Header().userToken, token);
    msg.SerializeMessage();

    EXPECT_EQ(strlen(token) + strlen("KeepAlive") + base_message::HeaderMinLength, msg.Header().headerLength);

    EXPECT_FALSE(msg.HasError());
    EXPECT_NE(0, msg.Header().crc);

    BaseMessage outMsg{msgBuffer};
    outMsg.DeserializeMessage();
    EXPECT_EQ(outMsg.LastError(), BaseMessage::MessageError::None);
    EXPECT_EQ(msg.Header().synchWord, outMsg.Header().synchWord);
    EXPECT_EQ(msg.Header().headerLength, outMsg.Header().headerLength);
    EXPECT_EQ(msg.Header().bodyLength, outMsg.Header().bodyLength);
    EXPECT_EQ(msg.Header().counter, outMsg.Header().counter);
    EXPECT_EQ(msg.Header().timeStamp, outMsg.Header().timeStamp);
    EXPECT_TRUE(0 == memcmp(msg.Header().version, outMsg.Header().version, VersionLength));
    EXPECT_EQ(msg.Header().typeOfMessage, outMsg.Header().typeOfMessage);
    EXPECT_EQ(msg.Header().totalParts, outMsg.Header().totalParts);
    EXPECT_EQ(msg.Header().currentPartIndex, outMsg.Header().currentPartIndex);
    EXPECT_EQ(msg.Header().crc, outMsg.Header().crc);
    EXPECT_EQ(0, strcmp(token, outMsg.Header().userToken));
}

TEST_F(BaseMessageTest, resetMessage) {
    BaseMessage msg{msgBuffer};
    msg.Header().timeStamp = 200;
    msg.Header().counter = 400;
    msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
    msg.Header().totalParts = 3;
    msg.Header().currentPartIndex = 2;
    msg.Header().crc = 0xFF;
    strcpy(msg.Header().userToken, "This is a fake token");
    msg.Reset();

    EXPECT_EQ(msg.Header().currentPartIndex, 0);
    EXPECT_EQ(msg.Header().totalParts, 1);
    EXPECT_EQ(msg.Header().typeOfMessage, base_message::MessageType::UnknownType);
    EXPECT_EQ(msg.Header().counter, 0);
    EXPECT_EQ(msg.Header().timeStamp, 0);
    EXPECT_EQ(msg.Header().crc, 0);
    EXPECT_EQ(strlen(msg.Header().userToken), 0);
}

TEST_F(BaseMessageTest, checkSyncWord) {
    BaseMessage msg{msgBuffer};
    msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
    msg.SerializeMessage();

    BaseMessage messedUp{msgBuffer};
    memset(msgBuffer, 0, 4);
    messedUp.DeserializeMessage();
    EXPECT_EQ(messedUp.LastError(), BaseMessage::MessageError::SynchWord);
}

TEST_F(BaseMessageTest, checkPartIndex) {
    const uint32_t tooLarge = 99;
    BaseMessage msg{msgBuffer};
    msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
    msg.Header().currentPartIndex = tooLarge;
    msg.SerializeMessage();
    EXPECT_EQ(msg.LastError(), BaseMessage::MessageError::PartIndexTooLarge);

    msg.Header().currentPartIndex = 0;
    msg.SerializeMessage();
    memcpy(msg.Buffer() + 44, &tooLarge, sizeof(tooLarge));
    ReCRC(msg);

    BaseMessage messedUp{msg.Buffer()};
    messedUp.DeserializeMessage();
    EXPECT_EQ(messedUp.Header().currentPartIndex, tooLarge);
    EXPECT_EQ(messedUp.LastError(), BaseMessage::MessageError::PartIndexTooLarge);
}

TEST_F(BaseMessageTest, checkDataLength) {
    BaseMessage msg{msgBuffer};
    msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
    uint32_t tooLarge = base_message::MessageLength;
    msg.Header().bodyLength = tooLarge;
    msg.SerializeMessage();
    EXPECT_EQ(msg.LastError(), BaseMessage::MessageError::DataLengthOverflow);
    msg.Header().bodyLength = 0;
    msg.SerializeMessage();
    memcpy(msgBuffer + 8, &tooLarge, 4);
    ReCRC(msg);
    BaseMessage messedUp{msgBuffer};
    messedUp.DeserializeMessage();
    EXPECT_EQ(messedUp.LastError(), BaseMessage::MessageError::DataLengthOverflow);
}

TEST_F(BaseMessageTest, checkMessageType) {
    std::vector<base_message::MessageType> unknowns{
        base_message::MessageType::UnknownType,
        base_message::MessageType::UnknownEvent,
        base_message::MessageType::UnknownTable,
        base_message::MessageType::UnknownMessage,
        base_message::MessageType::UnknownSerializeable,
    };

    BaseMessage msg{msgBuffer};

    for (const auto& unknown: unknowns) {
        msg.Header().typeOfMessage = unknown;
        msg.SerializeMessage();
        EXPECT_EQ(msg.LastError(), BaseMessage::MessageError::UnknownType);
        msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
        msg.SerializeMessage();
        auto unknownStr = MessageTypeToString(unknown);
        unknownStr.Serialize(msgBuffer + 56);
        ReCRC(msg);
        BaseMessage messedUp{msgBuffer};
        messedUp.DeserializeMessage();
        EXPECT_EQ(messedUp.LastError(), BaseMessage::MessageError::UnknownType);
    }
}

TEST_F(BaseMessageTest, unterminatedToken) {
    BaseMessage msg{msgBuffer};
    msg.Header().typeOfMessage = base_message::MessageType::KeepAliveType;
    memset(msg.Header().userToken, 0xFF, sizeof(msg.Header().userToken));
    msg.SerializeMessage();
    EXPECT_EQ(msg.LastError(), BaseMessage::MessageError::DataLengthOverflow);

    *(msg.Header().userToken + TokenMaxLength) = 0;
    msg.SerializeMessage();
    *(msgBuffer + msg.Header().headerLength - 2 - 16) = 0xFF; //this is for header length of 75. for future use - as increasing header's size in X byes -> decrease the same number of bytes
    ReCRC(msg);
    BaseMessage messedUp{msgBuffer};
    messedUp.DeserializeMessage();
    EXPECT_EQ(messedUp.LastError(), BaseMessage::MessageError::DataLengthOverflow);
}

TEST_F(BaseMessageTest, messageHasCorrectMaxSize) {
    const uint32_t len = TestReq::MaxSerializedLength;
    ASSERT_EQ(len,
            1 + 1 + 2 + 4 + 8 +  // primitives
           (4 + TestReq::MaxInternalBuffSize) + (4 + TestReq::MaxExternalBuffSize) + TestReq::ConstLengthBuffLen + // buffers
           TestReq::MaxStringLen + 1 + //string
           ((1 + TestReq::MaxSetStringLen) * TestReq::MaxStringsInSet + 4) + ((4 + TestReq::MaxDataLen) * TestReq::MaxDatas + 4) + // sets
           (3 * (EnumMaxLength + 1)) + // enums
           (4 * 5 + 4) + (5 + 4) + // primitive sets
           2 * ((EnumMaxLength + 1) * 5 + 4) + // enum sets
           32 // uint256
           );
}

TEST_F(BaseMessageTest, nestedMessageHasCorrectMaxSize) {
    const uint32_t len = TestNested::MaxSerializedLength;
    const uint32_t nestedLen = TestReq::MaxSerializedLength * 4 + 4;
    ASSERT_EQ(len, nestedLen + 8);
}

TEST_F(BaseMessageTest, serializeABody) {
    TestReqMessage msg{msgBuffer};
    SetTestReq(msg.Body());

    EXPECT_EQ(msg.Body().ADataSet().length, 4);
    EXPECT_EQ(msg.Body().SomeStrings().length, 5);

    msg.SerializeMessage();
    ASSERT_FALSE(msg.HasError());

    // I manually added this up - theres probably a better way to do it but whatever
    EXPECT_EQ(msg.Header().bodyLength, 185);

    BaseMessage baseOutMsg{msgBuffer};
    baseOutMsg.DeserializeMessage();
    ASSERT_FALSE(baseOutMsg.HasError());
    EXPECT_EQ(baseOutMsg.Header().bodyLength, 185);
    ASSERT_EQ(baseOutMsg.Header().typeOfMessage, base_message::MessageType::TestReqType);

    TestReqMessage outMsg{msgBuffer};
    outMsg.Body().ExternalBuff() = {externalBuff, sizeof(externalBuff)};
    outMsg.DeserializeMessage();
    ASSERT_FALSE(outMsg.HasError());

    EXPECT_EQ(outMsg.Body().UintEight(), msg.Body().UintEight());
    EXPECT_EQ(outMsg.Body().UintSixteen(), msg.Body().UintSixteen());
    EXPECT_EQ(outMsg.Body().UintThirtyTwo(), msg.Body().UintThirtyTwo());
    EXPECT_EQ(outMsg.Body().UintSixtyFour(), msg.Body().UintSixtyFour());
    EXPECT_EQ(outMsg.Body().Enam(), msg.Body().Enam());
    EXPECT_EQ(outMsg.Body().GlobalEnum(), msg.Body().GlobalEnum());
    EXPECT_EQ(outMsg.Body().MessageTypeEnum(), msg.Body().MessageTypeEnum());
    EXPECT_EQ(outMsg.Body().Baalean(), msg.Body().Baalean());
    EXPECT_TRUE(0 == strcmp(outMsg.Body().Strang(), msg.Body().Strang()));
    EXPECT_TRUE(0 == memcmp(outMsg.Body().InternalBuff().buffer, msg.Body().InternalBuff().buffer, outMsg.Body().InternalBuff().length));
    EXPECT_TRUE(0 == memcmp(outMsg.Body().ExternalBuff().buffer, msg.Body().ExternalBuff().buffer, outMsg.Body().ExternalBuff().length));
    EXPECT_TRUE(0 == memcmp(outMsg.Body().ConstLengthBuff().data(), msg.Body().ConstLengthBuff().data(), 6));

    EXPECT_EQ(4, outMsg.Body().ADataSet().length);
    for (uint32_t i = 0; i < outMsg.Body().ADataSet().length; i++) {
        auto& inItem = msg.Body().ADataSet()[i];
        auto& outItem = outMsg.Body().ADataSet()[i];
        EXPECT_EQ(inItem.length, outItem.length);
        EXPECT_TRUE(0 == memcmp(inItem.buffer, outItem.buffer, inItem.length));
    }

    EXPECT_EQ(5, outMsg.Body().SomeStrings().length);
    for (uint32_t i = 0; i < outMsg.Body().SomeStrings().length; i++) {
        auto& inItem = msg.Body().SomeStrings()[i];
        auto& outItem = outMsg.Body().SomeStrings()[i];
        EXPECT_TRUE(0 == strcmp(inItem.data(), outItem.data()));
    }

    EXPECT_EQ(2, outMsg.Body().PrimitiveSetOfUintEight().length);
    for (uint32_t i = 0; i < outMsg.Body().PrimitiveSetOfUintEight().length; i++) {
        auto& in = msg.Body().PrimitiveSetOfUintEight()[i];
        auto& out = outMsg.Body().PrimitiveSetOfUintEight()[i];
        EXPECT_EQ(in, out);
    }

    EXPECT_EQ(2, outMsg.Body().PrimitiveSetOfUintThirtyTwo().length);
    for (uint32_t i = 0; i < outMsg.Body().PrimitiveSetOfUintThirtyTwo().length; i++) {
        auto& in = msg.Body().PrimitiveSetOfUintThirtyTwo()[i];
        auto& out = outMsg.Body().PrimitiveSetOfUintThirtyTwo()[i];
        EXPECT_EQ(in, out);
    }

    EXPECT_EQ(2, outMsg.Body().SetOfEnum().length);
    for (uint32_t i = 0; i < outMsg.Body().SetOfEnum().length; i++) {
        auto& in = msg.Body().SetOfEnum()[i];
        auto& out = outMsg.Body().SetOfEnum()[i];
        EXPECT_EQ(in, out);
    }

    EXPECT_EQ(2, outMsg.Body().SetOfGlobalEnum().length);
    for (uint32_t i = 0; i < outMsg.Body().SetOfGlobalEnum().length; i++) {
        auto& in = msg.Body().SetOfGlobalEnum()[i];
        auto& out = outMsg.Body().SetOfGlobalEnum()[i];
        EXPECT_EQ(in, out);
    }
}

TEST_F(BaseMessageTest, resetMessageWithBody) {
    TestReqMessage msg{msgBuffer};
    SetTestReq(msg.Body());
    msg.SerializeMessage();
    msg.Reset();
    EXPECT_EQ(msg.Body().Baalean(), false);
    EXPECT_EQ(msg.Body().Baalean(), false);
    EXPECT_EQ(msg.Body().UintEight(), 0);
    EXPECT_EQ(msg.Body().UintSixteen(), 0);
    EXPECT_EQ(msg.Body().UintThirtyTwo(), 0);
    EXPECT_EQ(msg.Body().UintSixtyFour(), 0);
    EXPECT_EQ(msg.Body().Enam(), (TestReq::Enam_t)0);
    EXPECT_EQ(msg.Body().GlobalEnum(), (TestEnum)0);
    EXPECT_EQ(msg.Body().MessageTypeEnum(), (MessageType)0);
    EXPECT_EQ(msg.Body().ADataSet().length, 0);
    EXPECT_EQ(msg.Body().SomeStrings().length, 0);
    uint8_t zeros[200] = {0};
    EXPECT_TRUE(0 == strcmp("", msg.Body().Strang()));
    EXPECT_EQ(msg.Body().InternalBuff().length, 0);
    EXPECT_TRUE(0 == memcmp(zeros, msg.Body().InternalBuff().buffer, TestReq::MaxInternalBuffSize));
    EXPECT_EQ(msg.Body().ExternalBuff().buffer, nullptr);
    EXPECT_TRUE(0 == memcmp(zeros, msg.Body().ConstLengthBuff().data(), 6));

    for (auto& item : msg.Body().ADataSet().items) {
        EXPECT_EQ(0, item.length);
        EXPECT_TRUE(0 == memcmp(zeros, item.buffer, TestReq::MaxDataLen));
    }

    for (auto& item : msg.Body().SomeStrings().items) {
        EXPECT_TRUE(0 == memcmp(zeros, item.data(), TestReq::MaxStringLen + 1));
    }

    EXPECT_EQ(msg.Header().typeOfMessage, base_message::MessageType::TestReqType);
    EXPECT_EQ(msg.Header().bodyLength, 0);
}

TEST_F(BaseMessageTest, serializeANestedBody) {
    TestNestedMessage msg{msgBuffer};
    SetTestReq(msg.Body().NestTestReq());
    msg.Body().SomeNumber(42);
    msg.Body().SomeOtherNumber(43);
    msg.Body().SerializeableSet().PushItem(msg.Body().NestTestReq());
    msg.Body().SerializeableSet().PushItem(msg.Body().NestTestReq());


    msg.SerializeMessage();
    ASSERT_FALSE(msg.HasError());

    // I manually added this up - theres probably a better way to do it but whatever
    EXPECT_EQ(msg.Header().bodyLength, 567);

    BaseMessage baseOutMsg{msgBuffer};
    baseOutMsg.DeserializeMessage();
    ASSERT_FALSE(baseOutMsg.HasError());
    EXPECT_EQ(baseOutMsg.Header().bodyLength, 567);
    ASSERT_EQ(baseOutMsg.Header().typeOfMessage, base_message::MessageType::TestNestedType);

    TestNestedMessage outMsg{msgBuffer};
    outMsg.Body().NestTestReq().ExternalBuff() = {externalBuff, 0};
    for (auto & tr : outMsg.Body().SerializeableSet().items) {
        tr.ExternalBuff() = {externalBuff, 0};
    }
    outMsg.DeserializeMessage();
    ASSERT_FALSE(outMsg.HasError());

    EXPECT_EQ(outMsg.Body().SomeNumber(), msg.Body().SomeNumber());
    EXPECT_EQ(outMsg.Body().SomeOtherNumber(), msg.Body().SomeOtherNumber());

    TestReq& out = outMsg.Body().NestTestReq();
    TestReq& in = msg.Body().NestTestReq();

    EXPECT_EQ(out.UintEight(), in.UintEight());
    EXPECT_EQ(out.UintSixteen(), in.UintSixteen());
    EXPECT_EQ(out.UintThirtyTwo(), in.UintThirtyTwo());
    EXPECT_EQ(out.UintSixtyFour(), in.UintSixtyFour());
    EXPECT_EQ(out.Enam(), in.Enam());
    EXPECT_EQ(out.GlobalEnum(), in.GlobalEnum());
    EXPECT_EQ(out.MessageTypeEnum(), in.MessageTypeEnum());
    EXPECT_EQ(out.Baalean(), in.Baalean());
    EXPECT_TRUE(0 == strcmp(out.Strang(), in.Strang()));

    EXPECT_TRUE(0 == memcmp(out.InternalBuff().buffer, in.InternalBuff().buffer, out.InternalBuff().length));
    EXPECT_TRUE(0 == memcmp(out.ExternalBuff().buffer, in.ExternalBuff().buffer, out.ExternalBuff().length));
    EXPECT_TRUE(0 == memcmp(out.ConstLengthBuff().data(), in.ConstLengthBuff().data(), 6));

    for (uint32_t i = 0; i < out.ADataSet().length; i++) {
        auto& inItem = in.ADataSet()[i];
        auto& outItem = in.ADataSet()[i];
        EXPECT_EQ(inItem.length, outItem.length);
        EXPECT_TRUE(0 == memcmp(inItem.buffer, outItem.buffer, inItem.length));
    }

    for (uint32_t i = 0; i < out.SomeStrings().length; i++) {
        auto& inItem = in.SomeStrings()[i];
        auto& outItem = in.SomeStrings()[i];
        EXPECT_TRUE(0 == strcmp(inItem.data(), outItem.data()));
    }

    for (size_t i = 0; i < outMsg.Body().SerializeableSet().length; i++) {
        TestReq& outReq = outMsg.Body().SerializeableSet().items[i];
        TestReq& inReq = msg.Body().SerializeableSet().items[i];

        EXPECT_EQ(outReq.UintEight(), inReq.UintEight());
        EXPECT_EQ(outReq.UintSixteen(), inReq.UintSixteen());
        EXPECT_EQ(outReq.UintThirtyTwo(), inReq.UintThirtyTwo());
        EXPECT_EQ(outReq.UintSixtyFour(), inReq.UintSixtyFour());
        EXPECT_EQ(outReq.Enam(), inReq.Enam());
        EXPECT_EQ(outReq.GlobalEnum(), inReq.GlobalEnum());
        EXPECT_EQ(outReq.MessageTypeEnum(), inReq.MessageTypeEnum());
        EXPECT_EQ(outReq.Baalean(), inReq.Baalean());
        EXPECT_TRUE(0 == strcmp(outReq.Strang(), inReq.Strang()));

        EXPECT_TRUE(0 == memcmp(outReq.InternalBuff().buffer, inReq.InternalBuff().buffer, outReq.InternalBuff().length));
        EXPECT_TRUE(0 == memcmp(outReq.ExternalBuff().buffer, inReq.ExternalBuff().buffer, outReq.ExternalBuff().length));
        EXPECT_TRUE(0 == memcmp(outReq.ConstLengthBuff().data(), inReq.ConstLengthBuff().data(), 6));

        for (uint32_t i = 0; i < outReq.ADataSet().length; i++) {
            auto& inItem = inReq.ADataSet()[i];
            auto& outItem = inReq.ADataSet()[i];
            EXPECT_EQ(inItem.length, outItem.length);
            EXPECT_TRUE(0 == memcmp(inItem.buffer, outItem.buffer, inItem.length));
        }

        for (uint32_t i = 0; i < outReq.SomeStrings().length; i++) {
            auto& inItem = inReq.SomeStrings()[i];
            auto& outItem = inReq.SomeStrings()[i];
            EXPECT_TRUE(0 == strcmp(inItem.data(), outItem.data()));
        }

        for (uint32_t i = 0; i < outReq.PrimitiveSetOfUintEight().length; i++) {
            auto& inItem = inReq.PrimitiveSetOfUintEight()[i];
            auto& outItem = outReq.PrimitiveSetOfUintEight()[i];
            EXPECT_EQ(inItem, outItem);
        }

        for (uint32_t i = 0; i < outReq.PrimitiveSetOfUintThirtyTwo().length; i++) {
            auto& inItem = inReq.PrimitiveSetOfUintThirtyTwo()[i];
            auto& outItem = outReq.PrimitiveSetOfUintThirtyTwo()[i];
            EXPECT_EQ(inItem, outItem);
        }

        for (uint32_t i = 0; i < outReq.SetOfEnum().length; i++) {
            auto& inItem = inReq.SetOfEnum()[i];
            auto& outItem = outReq.SetOfEnum()[i];
            EXPECT_EQ(inItem, outItem);
        }

        for (uint32_t i = 0; i < outReq.SetOfGlobalEnum().length; i++) {
            auto& inItem = inReq.SetOfGlobalEnum()[i];
            auto& outItem = outReq.SetOfGlobalEnum()[i];
            EXPECT_EQ(inItem, outItem);
        }
    }

    EXPECT_EQ(2, out.PrimitiveSetOfUintEight().length);
    for (uint32_t i = 0; i < out.PrimitiveSetOfUintEight().length; i++) {
        auto& inItem = in.PrimitiveSetOfUintEight()[i];
        auto& outItem = out.PrimitiveSetOfUintEight()[i];
        EXPECT_EQ(inItem, outItem);
    }

    EXPECT_EQ(2, out.PrimitiveSetOfUintThirtyTwo().length);
    for (uint32_t i = 0; i < out.PrimitiveSetOfUintThirtyTwo().length; i++) {
        auto& inItem = in.PrimitiveSetOfUintThirtyTwo()[i];
        auto& outItem = out.PrimitiveSetOfUintThirtyTwo()[i];
        EXPECT_EQ(inItem, outItem);
    }

    EXPECT_EQ(2, out.SetOfEnum().length);
    for (uint32_t i = 0; i < out.SetOfEnum().length; i++) {
        auto& inItem = in.SetOfEnum()[i];
        auto& outItem = out.SetOfEnum()[i];
        EXPECT_EQ(inItem, outItem);
    }

    EXPECT_EQ(2, out.SetOfGlobalEnum().length);
    for (uint32_t i = 0; i < out.SetOfGlobalEnum().length; i++) {
        auto& inItem = in.SetOfGlobalEnum()[i];
        auto& outItem = out.SetOfGlobalEnum()[i];
        EXPECT_EQ(inItem, outItem);
    }
}

TEST_F(BaseMessageTest, canDeserializeMessagesWithExtendedHeaders) {
    BaseMessage msg{msgBuffer};
    msg.Header().timeStamp = 200;
    msg.Header().counter = 400;
    msg.Header().typeOfMessage = MessageType::KeepAliveType;
    msg.Header().totalParts = 3;
    msg.Header().currentPartIndex = 2;
    const char* token = "Wow check out this amazing token";
    strcpy(msg.Header().userToken, token);
    msg.SerializeMessage();
    ASSERT_FALSE(msg.HasError());
    uint32_t originalHeaderLength = msg.Header().headerLength;
    *(uint32_t*)(msgBuffer + 4) = originalHeaderLength + 4;
    msg.Header().headerLength += 4;
    *(uint32_t*)(msgBuffer + originalHeaderLength) = 42;
    msg.Header().crc = msg.CalculateCRC();
    *(crc32_t*)(msgBuffer + MessageCrcIndex) = msg.Header().crc;

    msg.DeserializeMessage();
    ASSERT_EQ(strcmp(msg.Header().userToken, token), 0);
    ASSERT_EQ(originalHeaderLength + 4, msg.Header().headerLength);
    ASSERT_EQ(BaseMessage::MessageError::None, msg.LastError());
}

TEST_F(BaseMessageTest, copyConstructorIsFriendly) {
    TestReq req;
    req.Enam(TestReq::Enam_t::Test);
    TestReqMessage message{req};
    ASSERT_EQ(message.Body().Enam(), TestReq::Enam_t::Test);
}

TEST_F(BaseMessageTest, compileFlagsWork) {
    TestCompileFlagsCCMessage cc;
    TestCompileFlagsCryptoMessage crypto;
}
