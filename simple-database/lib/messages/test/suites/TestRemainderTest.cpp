#include <gtest/gtest.h>
#include "TestRemainder.hpp"

class TestRemainderTest : public ::testing::Test {
    protected:
        virtual void SetUp() {
        }

        virtual void TearDown() {
        }

        TestRemainder testRemainder;
};

TEST_F(TestRemainderTest, lengthWasCalculatedCorrectly) {
    uint32_t calculated = TestRemainder::MaxBuffSize;
    ASSERT_EQ(calculated, base_message::MessageLength - base_message::HeaderMinLength - base_message::EnumMaxLength - 4);
}

TEST_F(TestRemainderTest, maxSizeWasCalculatedCorrectly) {
    uint32_t calculated = TestRemainder::MaxSerializedLength;
    ASSERT_EQ(calculated, base_message::MessageLength - base_message::HeaderMinLength - base_message::EnumMaxLength);
}
