#include <gtest/gtest.h>
#include "Property.hpp"
#include "LengthEncodedSet.hpp"

class PropertyTest : public ::testing::Test {
    public:
    enum Enum {
        Test,
        Production,
    };
    class EnumWrapper : public BaseEnumWrapper<Enum> {
        public:
        EnumWrapper() = default;
        EnumWrapper(Enum e) { value = e; }
        FixedLengthString<base_message::EnumMaxLength> ToString() const override {
            switch(value) {
                case Enum::Test:
                    return "Test";
                case Enum::Production:
                    return "Production";
            }
            return "";
        }
        void FromString(const EnumString &str) override {
            if (str == "Test") {
                value = Enum::Test;
            }

            if (str == "Production") {
                value = Enum::Production;
            }
        }
    };
};

TEST_F(PropertyTest, transferUint8) {
    uint8_t i = 44;
    uint8_t j = 23;
    auto prop = PrimitiveProperty("Name", uint8_t);
    prop.Deserialize(&i);
    prop.Serialize(&j);
    EXPECT_EQ(44, j);
};

TEST_F(PropertyTest, transferUint16) {
    uint16_t i = 440;
    uint16_t j = 230;
    auto prop = PrimitiveProperty("Name", uint16_t);
    prop.Deserialize(&i);
    prop.Serialize(&j);
    EXPECT_EQ(440, j);
};

TEST_F(PropertyTest, transferUint32) {
    uint32_t i = 4400000;
    uint32_t j = 290821;
    auto prop = PrimitiveProperty("Name", uint32_t);
    prop.Deserialize(&i);
    prop.Serialize(&j);
    EXPECT_EQ(4400000, j);
};

TEST_F(PropertyTest, transferUint64) {
    uint64_t i = 440000000000;
    uint64_t j = 290821830000;
    auto prop = PrimitiveProperty("Name", uint64_t);
    prop.Deserialize(&i);
    prop.Serialize(&j);
    EXPECT_EQ(440000000000, j);
};

TEST_F(PropertyTest, transferBool) {
    bool i = true;
    bool j = false;
    auto prop = PrimitiveProperty("Name", bool);
    prop.Deserialize(&i);
    prop.Serialize(&j);
    EXPECT_EQ(true, j);
};

TEST_F(PropertyTest, transferEnum) {
    EnumString i = "Test";
    EnumString j = "Production";
    auto prop = EnumProperty("Name", EnumWrapper);
    prop.Deserialize((char*)i);
    prop.Serialize(j.data());
    EXPECT_EQ(0, strcmp(i, j));
};

TEST_F(PropertyTest, transferInternalBuffer) {
    uint32_t n = 283948;
    uint8_t buff1[8];
    *(uint32_t*)buff1 = 4;
    *((uint32_t*)buff1 + 1) = n;
    uint8_t buff2[8] = {0xff};
    auto prop = InternalBufferProperty("Name", 4);
    prop.Deserialize(&buff1);
    prop.Serialize(&buff2);
    EXPECT_EQ(*(uint32_t*)buff2, 4);
    EXPECT_EQ(*((uint32_t*)buff2 + 1), n);
};

TEST_F(PropertyTest, transferExternalBuffer) {
    uint32_t n = 283954;
    uint8_t buff1[8];
    *(uint32_t*)buff1 = 4;
    *((uint32_t*)buff1 + 1) = n;
    uint8_t buff2[8] = {0xff};
    auto prop = ExternalBufferProperty("Name", 4);
    uint8_t externalBuff[4];
    prop.data.SetBuffer(externalBuff);
    prop.Deserialize(&buff1);
    prop.Serialize(&buff2);
    EXPECT_EQ(*(uint32_t*)buff2, 4);
    EXPECT_EQ(*((uint32_t*)buff2 + 1), n);
};

TEST_F(PropertyTest, transferConstLengthBuffer) {
    uint32_t n = 283954;
    uint8_t buff1[4];
    *(uint32_t*)buff1 = n;
    uint8_t buff2[4] = {0xff};
    auto prop = ConstLengthBufferProperty("Name", 4);
    prop.Deserialize(&buff1);
    prop.Serialize(&buff2);
    EXPECT_EQ(*(uint32_t*)buff2, n);
};

TEST_F(PropertyTest, transferString) {
    char s[5] = "pop";
    char s2[5] = {'a', 'b', 'c', 'd', 'e'};
    auto prop = StringProperty("Name", 4);
    prop.Deserialize(s);
    prop.Serialize(s2);
    EXPECT_EQ('\0', s2[3]);
    EXPECT_TRUE(0 == strcmp(s2, "pop"));
};

TEST_F(PropertyTest, transferStringWithoutNullTermination) {
    char s[5] = {'a', 'b', 'c', 'd', 'e'};
    char s2[5] = {'e', 'd', 'c', 'b', 'a'};
    auto prop = StringProperty("Name", 4);
    prop.Deserialize(s);
    prop.Serialize(s2);
    EXPECT_EQ('\0', s2[4]);
    EXPECT_TRUE(0 == strcmp(s2, "abcd"));
};

TEST_F(PropertyTest, initFixedLengthStringWithCharPtr) {
    FixedLengthString<5> s = "Hello";
    EXPECT_EQ(s.TotalLength(), strlen("Hello") + 1);
    EXPECT_EQ(0, strcmp(s, "Hello"));

    FixedLengthString<10> bigS = "Hello";
    EXPECT_EQ(s.TotalLength(), strlen("Hello") + 1);
    EXPECT_EQ(0, strcmp(bigS, "Hello"));

    char str[10] = "Hello";
    bigS = str;
    EXPECT_EQ(s.TotalLength(), strlen("Hello") + 1);
    EXPECT_EQ(0, strcmp(bigS, "Hello"));
};

TEST_F(PropertyTest, transferLengthEncodedSet) {
    LengthEncodedSet<4, 4> set;

    uint8_t item[2];
    *(uint16_t*)item = 0xDEAD;
    set.PushItem({item, 2});
    *item = 0xEE;
    set.PushItem({item, 1});

    uint8_t in[100] = {0xff};
    set.Serialize(in);

    uint8_t out[100] = {0xff};

    auto prop = LengthEncodedSetProperty("Name", 4, 4);
    prop.Deserialize(in);
    prop.Serialize(out);
    EXPECT_TRUE(0 == memcmp(out, in, set.TotalLength()));
};

TEST_F(PropertyTest, transferStringSet) {
    StringSet<4, 4> set;
    char in[100];

    set.PushItem("yo");
    set.PushItem("p");
    set.Serialize((uint8_t*)in);

    char out[100];

    auto prop = StringSetProperty("Name", 4, 4);
    prop.Deserialize(in);
    prop.Serialize(out);
    EXPECT_TRUE(0 == memcmp(out, in, set.TotalLength()));
};

TEST_F(PropertyTest, transferPrimitiveSet) {
    LengthPrefixedSet<uint32_t, 4> set;
    char in[100];

    set.PushItem(3);
    set.PushItem(5);
    set.Serialize(in);

    char out[100];

    auto prop = PrimitiveSetProperty("Name", uint32_t, 4);
    prop.Deserialize(in);
    prop.Serialize(out);
    EXPECT_TRUE(0 == memcmp(out, in, set.TotalLength()));
};

TEST_F(PropertyTest, transferUint256Buffer) {
    uint8_t in[32];
    memset(in, 0xAA, 32);
    uint8_t out[32];
    auto prop = UInt256Property("Name");
    prop.Deserialize(&in);
    prop.Serialize(&out);
    EXPECT_TRUE(0 == memcmp(out, in, prop.TotalLength()));
};

TEST_F(PropertyTest, fixedLengthStringEquivalenceOperator) {
    FixedLengthString<22> str1{"hello my name is string"};
    FixedLengthString<22> str2{"hello my name is string"};
    FixedLengthString<22> str3{"hello my name is strang"};
    EXPECT_TRUE(str1 == str2);
    EXPECT_TRUE(str1 == "hello my name is string");
    EXPECT_TRUE(str1 != str3);
    EXPECT_TRUE(str1 != "hello my name is strang");
    EXPECT_FALSE(str1 == str3);
    EXPECT_FALSE(str1 == "hello my name is strang");
    EXPECT_FALSE(str1 != str2);
    EXPECT_FALSE(str1 != "hello my name is string");
}

TEST_F(PropertyTest, constLengthBufferEquivalenceOperator) {
    uint8_t boof[22] = {0xff};
    ConstLengthBuffer<22> buf1;
    buf1.Set(boof);
    ConstLengthBuffer<22> buf2;
    buf2.Set(boof);
    ConstLengthBuffer<22> buf3;
    boof[0] = 0xda;
    buf3.Set(boof);
    EXPECT_TRUE(buf1 == buf2);
    EXPECT_TRUE(buf1 != buf3);
    EXPECT_FALSE(buf1 != buf2);
    EXPECT_FALSE(buf1 == buf3);
}

TEST_F(PropertyTest, lengthPrefixedBufferEquivalenceOperator) {
    uint8_t boof[20] = {0xff};
    LengthPrefixedBuffer<22> buf1;
    buf1.Set(boof, sizeof(boof));
    LengthPrefixedBuffer<22> buf2;
    buf2.Set(boof, sizeof(boof));
    LengthPrefixedBuffer<22> buf3;
    boof[0] = 0xda;
    buf3.Set(boof, sizeof(boof));
    EXPECT_TRUE(buf1 == buf2);
    EXPECT_TRUE(buf1 != buf3);
    EXPECT_FALSE(buf1 != buf2);
    EXPECT_FALSE(buf1 == buf3);
}

TEST_F(PropertyTest, lengthPrefixedExternalBufferEquivalenceOperator) {
    uint8_t boof[20] = {0xff};
    LengthPrefixedExternalBuffer<22> buf1;
    uint8_t extBuf1[22];
    buf1.SetBuffer(extBuf1);
    buf1.Set(boof, sizeof(boof));
    LengthPrefixedExternalBuffer<22> buf2;
    uint8_t extBuf2[22];
    buf2.SetBuffer(extBuf2);
    buf2.Set(boof, sizeof(boof));
    LengthPrefixedExternalBuffer<22> buf3;
    uint8_t extBuf3[22];
    buf3.SetBuffer(extBuf3);
    uint8_t beef[20] = {0xda};
    buf3.Set(beef, sizeof(beef));
    EXPECT_TRUE(buf1 == buf2);
    EXPECT_TRUE(buf1 != buf3);
    EXPECT_FALSE(buf1 != buf2);
    EXPECT_FALSE(buf1 == buf3);
}
