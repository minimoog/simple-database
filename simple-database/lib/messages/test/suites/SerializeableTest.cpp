#include <gtest/gtest.h>
#include "Serializeable.hpp"
#include "TestReq.hpp"

class SerializeableTest : public ::testing::Test {
    protected:
        void SetUp() {
            memset(buffer, 0, sizeof(buffer));
            SetContent(tr);
        }

        void SetContent(TestReq& tr) {
            tr.UintEight(0xFF - 1);
            tr.UintSixteen(0xFFFF - 1);
            tr.UintThirtyTwo(0xFFFFFFFF - 1);
            tr.UintSixtyFour(0xFFFFFFFFFFFFFFFF - 1);
            tr.Baalean(true);
            tr.Strang("yo");
            tr.Enam(TestReq::Enam_t::Test);
            tr.GlobalEnum(TestEnum::Two);

            uint8_t internalBuff[TestReq::MaxInternalBuffSize];
            memset(internalBuff, 0xD5, sizeof(internalBuff));
            tr.InternalBuff(internalBuff, sizeof(internalBuff));

            memset(externalBuff + 4, 0x55, sizeof(externalBuff) - 4);
            tr.ExternalBuff() = {externalBuff, sizeof(externalBuff)};

            uint8_t constLengthBuff[TestReq::ConstLengthBuffLen];
            memset(constLengthBuff, 0xEE, sizeof(constLengthBuff));
            tr.ConstLengthBuff(constLengthBuff);

            uint8_t uint256Buff[32];
            memset(uint256Buff, 0xFF, sizeof(uint256Buff));
            tr.UIntTwoFiveSix(uint256Buff);

            std::vector<const char*> data {
                "feed",
                "me",
                "see",
                "more",
            };
            for (const auto& d: data) {
                tr.ADataSet().PushItem({(uint8_t*)d, (uint32_t)strlen(d)});
            }

            std::vector<const char*> strings {
                "you",
                "can",
                "do",
                "it",
                "!",
            };
            for (const auto& s: strings) {
                tr.SomeStrings().PushItem(s);
            }

            tr.PrimitiveSetOfUintThirtyTwo().PushItem(1);
            tr.PrimitiveSetOfUintThirtyTwo().PushItem(2);

            tr.PrimitiveSetOfUintEight().PushItem(2);
            tr.PrimitiveSetOfUintEight().PushItem(3);
        }

        void TearDown() {
            tr.Reset();
        }

        TestReq tr;
        uint8_t buffer[TestReq::MaxSerializedLength];
        uint8_t externalBuff[TestReq::MaxExternalBuffSize];
};

TEST_F(SerializeableTest, EqualOperatorTrueWhenSame) {
    TestReq copyTr;
    SetContent(copyTr);
    ASSERT_TRUE(tr == copyTr);
    ASSERT_FALSE(tr != copyTr);
    copyTr.Baalean(false);
    ASSERT_TRUE(tr != copyTr);
    ASSERT_FALSE(tr == copyTr);
}

TEST_F(SerializeableTest, baseClassCanSerializeNothing) {
    uint8_t buff[20] = {0xed};
    uint8_t buff2[20] = {0xed};
    Serializeable s;
    uint32_t len = s.Serialize(buff);
    EXPECT_EQ(len, 0);
    EXPECT_TRUE(0 == memcmp(buff, buff2, sizeof(buff)));
    s.Reset();
    EXPECT_TRUE(0 == memcmp(buff, buff2, sizeof(buff)));
    const BaseProperty* prop = s.PropertyAtIndex(0);
    EXPECT_EQ(nullptr, prop);
}

TEST_F(SerializeableTest, testReqHasExpectedProperties)
{
    uint32_t nProps = tr.NumberOfProperties();
    ASSERT_EQ(nProps, 19);
    uint32_t maxLength = tr.MaxLength();
    const uint32_t maxConst = TestReq::MaxSerializedLength;
    EXPECT_EQ(maxLength, maxConst);
    EXPECT_EQ(maxLength, 593);

    size_t i = 0;
    const BaseProperty* prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("UintEight", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::UInt8, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsPrimitive(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("UintSixteen", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::UInt16, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsPrimitive(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("UintThirtyTwo", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::UInt32, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsPrimitive(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("UintSixtyFour", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::UInt64, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsPrimitive(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("Baalean", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::Bool, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsPrimitive(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("Strang", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::String, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsBuffer(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("InternalBuff", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::InternalBuffer, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsBuffer(prop->Type()));
    EXPECT_EQ(TestReq::MaxInternalBuffSize + 4, prop->MaxLength());

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("ExternalBuff", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::ExternalBuffer, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsBuffer(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("ConstLengthBuff", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::ConstLengthBuffer, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsBuffer(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("ADataSet", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::LengthEncodedSet, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsSet(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("SomeStrings", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::StringSet, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsSet(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("Enam", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::Enum, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsPrimitive(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("GlobalEnum", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::Enum, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsPrimitive(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("MessageTypeEnum", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::Enum, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsPrimitive(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("PrimitiveSetOfUintThirtyTwo", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::PrimitiveSet, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsSet(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("PrimitiveSetOfUintEight", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::PrimitiveSet, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsSet(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("SetOfEnum", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::EnumSet, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsSet(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("SetOfGlobalEnum", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::EnumSet, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsSet(prop->Type()));

    i++;
    prop = tr.PropertyAtIndex(i);
    ASSERT_TRUE(0 == strcmp("UIntTwoFiveSix", prop->Name()));
    EXPECT_EQ(BaseProperty::PropertyType::UInt256, prop->Type());
    EXPECT_TRUE(BaseProperty::PropertyIsBuffer(prop->Type()));
};

TEST_F(SerializeableTest, serializeToNonCompactBuffer) {
    uint32_t l = tr.Serialize(buffer, false);
    EXPECT_EQ(l, tr.MaxLength());

    TestReq trOut;
    trOut.ExternalBuff() = {externalBuff, 0};
    trOut.Deserialize(buffer, false);

    EXPECT_EQ(trOut.UintEight(), tr.UintEight());
    EXPECT_EQ(trOut.UintSixteen(), tr.UintSixteen());
    EXPECT_EQ(trOut.UintThirtyTwo(), tr.UintThirtyTwo());
    EXPECT_EQ(trOut.UintSixtyFour(), tr.UintSixtyFour());
    EXPECT_EQ(trOut.Enam(), tr.Enam());
    EXPECT_EQ(trOut.GlobalEnum(), tr.GlobalEnum());
    EXPECT_EQ(trOut.Baalean(), tr.Baalean());
    EXPECT_TRUE(0 == strcmp(trOut.Strang(), tr.Strang()));
    EXPECT_TRUE(0 == memcmp(trOut.InternalBuff().buffer, tr.InternalBuff().buffer, trOut.InternalBuff().length));
    EXPECT_TRUE(0 == memcmp(trOut.ExternalBuff().buffer, tr.ExternalBuff().buffer, trOut.ExternalBuff().length));
    EXPECT_TRUE(0 == memcmp(trOut.ConstLengthBuff().data(), tr.ConstLengthBuff().data(), 6));

    for (uint32_t i = 0; i < trOut.ADataSet().length; i++) {
        auto& in = tr.ADataSet()[i];
        auto& out = trOut.ADataSet()[i];
        EXPECT_EQ(in.length, out.length);
        EXPECT_TRUE(0 == memcmp(in.buffer, out.buffer, in.length));
    }

    for (uint32_t i = 0; i < trOut.SomeStrings().length; i++) {
        auto& in = tr.SomeStrings()[i];
        auto& out = trOut.SomeStrings()[i];
        EXPECT_TRUE(0 == strcmp(in, out));
    }

    for (uint32_t i = 0; i < trOut.PrimitiveSetOfUintEight().length; i++) {
        auto& in = tr.PrimitiveSetOfUintEight()[i];
        auto& out = trOut.PrimitiveSetOfUintEight()[i];
        EXPECT_EQ(in, out);
    }

    for (uint32_t i = 0; i < trOut.PrimitiveSetOfUintThirtyTwo().length; i++) {
        auto& in = tr.PrimitiveSetOfUintThirtyTwo()[i];
        auto& out = trOut.PrimitiveSetOfUintThirtyTwo()[i];
        EXPECT_EQ(in, out);
    }
};

TEST_F(SerializeableTest, findPropertyByName) {
    const BaseProperty* prop = tr.PropertyByName("SomeStrings");
    EXPECT_TRUE(0 == strcmp(prop->Name(), "SomeStrings"));
    EXPECT_EQ(prop->Type(), BaseProperty::PropertyType::StringSet);
}

TEST_F(SerializeableTest, queryTheNonCompactBufferPosition) {
    uint32_t pos = tr.NonCompactPropertyPosition("UintEight");
    EXPECT_EQ(pos, 0);

    pos = tr.NonCompactPropertyPosition("GlobalEnum");
    EXPECT_EQ(pos, 148);
}

TEST_F(SerializeableTest, readAPropertyFromANonCompactBuffer) {
    uint8_t buffer[tr.MaxLength()];
    tr.Serialize(buffer, false);
    tr.Reset();

    size_t pos = tr.NonCompactPropertyPosition("SomeStrings");

    StringSet<TestReq::MaxStringsInSet, TestReq::MaxSetStringLen> set;
    set.Deserialize(buffer + pos);

    EXPECT_EQ(0, strcmp(set[0], "you"));
    EXPECT_EQ(0, strcmp(set[1], "can"));
    EXPECT_EQ(0, strcmp(set[2], "do"));
    EXPECT_EQ(0, strcmp(set[3], "it"));
    EXPECT_EQ(0, strcmp(set[4], "!"));

    tr.PropertyByName("SomeStrings")->Deserialize(buffer + pos);
    EXPECT_EQ(0, strcmp(tr.SomeStrings()[0], "you"));
    EXPECT_EQ(0, strcmp(tr.SomeStrings()[1], "can"));
    EXPECT_EQ(0, strcmp(tr.SomeStrings()[2], "do"));
    EXPECT_EQ(0, strcmp(tr.SomeStrings()[3], "it"));
    EXPECT_EQ(0, strcmp(tr.SomeStrings()[4], "!"));
};

TEST_F(SerializeableTest, nameIsStaticallyInitialized) {
    char name[] = "TestReq";
    EXPECT_TRUE(0 == strcmp(name, TestReq::SerializeableName));
};
