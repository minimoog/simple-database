#include <gtest/gtest.h>
#include "TestTable.hpp"

class TableTest : public ::testing::Test {
    protected:
        virtual void SetUp() {
        }

        virtual void TearDown() {
        }

};

TEST_F(TableTest, IdAndMetaAddedAutomatically) {
    TestTable table;
    ASSERT_EQ(8 + 8 + 13 + 8 + 8, TestTable::MaxSerializedLength);

    table.Id(1);
    ASSERT_EQ(1, table.Id());

    table.Metadata().SomeId(1);
    ASSERT_EQ(1, table.Metadata().SomeId());

    table.Metadata().Time(1);
    ASSERT_EQ(1, table.Metadata().Time());

    std::vector<uint8_t> v;
    v.resize(TestTable::MaxSerializedLength);
    auto s = table.Serialize(v.data());
    ASSERT_EQ(s, 8 + 8 + 1 + 8 + 8);
    TestTable table2;
    auto s2 = table2.Deserialize(v.data());
    ASSERT_EQ(s2, s);
    ASSERT_EQ(table, table2);
}
