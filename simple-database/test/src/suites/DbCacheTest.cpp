#include <gtest/gtest.h>
#include "DbCache.hpp"
#include "fs.hpp"

const size_t ItemSize = 1024;

class DbCacheTest : public ::testing::Test {
    protected:
    void SetUp() override {
        cache.Clear();
        cache.AddItem("one", data1, sizeof(data1));
        cache.AddItem("two", data2, sizeof(data2));
    }

    void TearDown() override {
    }

    const size_t MaxCacheSize = 3 * 1024;
    DbCache cache = {MaxCacheSize};
    const uint8_t data1[ItemSize] = {0xee, 0xaa, 0x7e, 0x9f};
    const uint8_t data2[ItemSize] = {0xd0, 0xad, 0xb9, 0x7f};
};

TEST_F(DbCacheTest, ItemCanBeCached) {
    const uint8_t data[ItemSize] = {0xde, 0xad, 0xbe, 0xef};
    cache.AddItem("key", data, sizeof(data));

    uint8_t cached[ItemSize];
    size_t len;
    ASSERT_TRUE(cache.GetItem("one", cached, len));
    ASSERT_TRUE(cache.GetItem("two", cached, len));
    ASSERT_TRUE(cache.GetItem("key", cached, len));
    ASSERT_EQ(len, ItemSize);
    ASSERT_EQ(0, memcmp(cached, data, sizeof(data)));
}

TEST_F(DbCacheTest, ItemCanBeRemoved) {
    uint8_t cached[ItemSize];
    size_t len;
    ASSERT_TRUE(cache.GetItem("one", cached, len));
    cache.RemoveItem("one");
    ASSERT_FALSE(cache.GetItem("one", cached, len));
}

TEST_F(DbCacheTest, DataNotModifiedIfKeyDoesntExist) {
    uint8_t cached[ItemSize];
    memcpy(cached, data1, sizeof(data1));
    size_t len;
    ASSERT_FALSE(cache.GetItem("nope", cached, len));
    ASSERT_EQ(0, memcmp(data1, cached, sizeof(data1)));
}

TEST_F(DbCacheTest, AllItemsCanBeCleared) {
    uint8_t cached[ItemSize];
    size_t len;
    ASSERT_TRUE(cache.GetItem("one", cached, len));
    ASSERT_TRUE(cache.GetItem("two", cached, len));
    cache.Clear();
    ASSERT_FALSE(cache.GetItem("one", cached, len));
    ASSERT_FALSE(cache.GetItem("two", cached, len));
}

TEST_F(DbCacheTest, CanAddDataAfterReachingTheLimit) {
    const uint8_t data[ItemSize] = {0xde, 0xad, 0xbe, 0xef};
    cache.AddItem("key1", data, sizeof(data));
    cache.AddItem("key2", data, sizeof(data));

    uint8_t cached[ItemSize];
    size_t len;
    ASSERT_TRUE(cache.GetItem("key2", cached, len));
    ASSERT_EQ(len, ItemSize);
    ASSERT_EQ(0, memcmp(cached, data, sizeof(data)));
}

TEST_F(DbCacheTest, LastInsertedItemIsPushedOutFirst) {
    const uint8_t data[] = {0xde, 0xad, 0xbe, 0xef};
    cache.AddItem("key1", data, sizeof(data));
    cache.AddItem("key2", data, sizeof(data));

    uint8_t cached[ItemSize];
    size_t len;
    ASSERT_FALSE(cache.GetItem("one", cached, len));
    ASSERT_TRUE(cache.GetItem("two", cached, len));
}


TEST_F(DbCacheTest, CanEraseMultipleItemsDuringSingleItemAdd) {
    const uint8_t data[] = {0xde, 0xad, 0xbe, 0xef};
    cache.AddItem("key1", data, sizeof(data));
    uint8_t cached[ItemSize];
    size_t len;
    ASSERT_TRUE(cache.GetItem("one", cached, len));
    ASSERT_TRUE(cache.GetItem("two", cached, len));
    ASSERT_TRUE(cache.GetItem("key1", cached, len));

    static const size_t largeDataLength = 3 * 1024;
    uint8_t largeData[largeDataLength] = {0};

    size_t size;

    cache.AddItem("largest-boy", largeData, 3 * 1024);
    ASSERT_FALSE(cache.GetItem("one", cached, len));
    ASSERT_FALSE(cache.GetItem("two", cached, len));
    ASSERT_FALSE(cache.GetItem("key1", cached, len));
    ASSERT_TRUE(cache.GetItem("largest-boy", largeData, len));
}
