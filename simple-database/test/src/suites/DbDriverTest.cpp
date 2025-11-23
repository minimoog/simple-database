#include <gtest/gtest.h>
#include "DbDriver.hpp"
#include "MessageEnums.hpp"
#include "fs.hpp"
#include "TestUser.hpp"
#include "Table.hpp"

class DbDriverInitTest : public ::testing::Test {
    protected:
        virtual void SetUp() override {
            initFS();
        }

        void CheckInit() {
            ASSERT_TRUE(DbDriver::InitDb());
            EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db").c_str()));
        }
};

class DbDriverTest : public DbDriverInitTest {
    protected:
        void SetUp() override {
            initFS();
            DirectoryWrapper::Delete(Path("/db").c_str());
            DbDriver::InitDb();

        }

        void TearDown() override {
            DirectoryWrapper::Delete(Path("/db").c_str());
            DbDriver::SetOnCreateCallback(nullptr);
            DbDriver::SetOnDeleteCallback(nullptr);
        }
 };


TEST_F(DbDriverInitTest, initDb) {
    CheckInit();
};

TEST_F(DbDriverTest, initTable) {
    DbDriver dbDriver{0, false};
    DirectoryWrapper dir;
    dbDriver.OpenTable("User", dir);
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/user").c_str()));
    dir.Close();
}

TEST_F(DbDriverTest, initScope) {
    DirectoryWrapper dir;
    DbDriver dbDriver{1, false};
    dbDriver.OpenTable("User", dir);
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/0100000000000000/user").c_str()));
    dir.Close();
}

TEST_F(DbDriverTest, canReInitDb) {
    CheckInit();
};

TEST_F(DbDriverTest, reInitPreservesData) {
    std::string path = Path("/db/user/0100000000000000");
    DirectoryWrapper::New(Path("/db/user").c_str());

    {
        FileWrapper f(path.c_str(), "w");
        ASSERT_TRUE(f.DidOpen());
    }

    CheckInit();

    FileWrapper f(path.c_str(), "r");
    ASSERT_TRUE(f.DidOpen());
}

TEST_F(DbDriverTest, objIdStartsAtOne) {
    ObjId id;
    DbDriver dbDriver{0, false};
    ASSERT_TRUE(dbDriver.NextId("User", id));
    EXPECT_EQ(id, 1);
};

TEST_F(DbDriverTest, objIdIncrements) {
    ObjId id;
    DbDriver dbDriver{0, false};
    ASSERT_TRUE(dbDriver.NextId("Users", id));
    ASSERT_TRUE(dbDriver.NextId("Users", id));
    EXPECT_EQ(id, 2);
};

TEST_F(DbDriverTest, objIdCanBePeekedAt) {
    ObjId id;
    DbDriver dbDriver{0, false};
    ASSERT_TRUE(dbDriver.NextId("Users", id));

    ObjId peek;
    dbDriver.NextId("Users", peek, false);

    ASSERT_TRUE(dbDriver.NextId("Users", id));
    EXPECT_EQ(id, 2);
    EXPECT_EQ(id, peek);
};

TEST_F(DbDriverTest, objIdIsSharedBetweenPendingAndMainTable) {
    ObjId id;
    DbDriver dbDriver{0, false};
    ASSERT_TRUE(dbDriver.NextId("Users", id));
    DbDriver dbDriver2{0, true};
    ASSERT_TRUE(dbDriver2.NextId("Users", id));
    EXPECT_EQ(id, 2);
};

TEST_F(DbDriverTest, savingRecordWithNextIdIncrementsCounter) {
    ObjId id;
    DbDriver dbDriver{0, false};
    ASSERT_TRUE(dbDriver.NextId("Users", id));

    ObjId peek;
    dbDriver.NextId("Users", peek, false);

    uint8_t data[10] = {0x55};
    dbDriver.SaveRecord(peek, 0, data, sizeof(data), "Users");

    dbDriver.NextId("Users", peek, false);
    EXPECT_EQ(peek, 3);

    dbDriver.SaveRecord(30, 0, data, sizeof(data), "Users");

    dbDriver.NextId("Users", peek, false);
    EXPECT_EQ(peek, 31);
};

TEST_F(DbDriverTest, recordsAreSavedWithCRC) {
    uint8_t data[10] = {0x55};
    DbDriver dbDriver{0, false};
    ASSERT_TRUE(dbDriver.SaveRecord(1, 0, data, sizeof(data), "User"));

    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/user/0100000000000000").c_str()));

    FileWrapper f(Path("/db/user/0100000000000000").c_str());
    uint32_t crc = 0;
    ObjId commitId = 0;
    ASSERT_EQ(f.Size(), sizeof(data) + sizeof(ObjId) + sizeof(crc));
    ASSERT_TRUE(f.Read(data, sizeof(data)));
    ASSERT_TRUE(f.Read(&commitId, sizeof(commitId)));
    ASSERT_EQ(commitId, 0);
    ASSERT_TRUE(f.Read(&crc, sizeof(crc)));
    ASSERT_EQ(crc, crc32(data, sizeof(data)));
}

TEST_F(DbDriverTest, corruptedDataCannotBeRead) {
    DirectoryWrapper::New(Path("/db/user").c_str());
    uint8_t data[100] = {0x55};

    {
        FileWrapper f(Path("/db/user/0100000000000000").c_str(), "w");
        ASSERT_TRUE(f.Write(data, sizeof(data)));
    }

    DbDriver::ClearCache();
    DbDriver dbDriver{0, false};
    EXPECT_FALSE(dbDriver.GetRecord(data, 1, "User") > 0);
}

TEST_F(DbDriverTest, writeUncommittedDataWithPreCommitId) {
    DbDriver dbDriver{0, true};
    const uint32_t dataLen = 10;
    uint8_t data[dataLen] = {0x55};
    ASSERT_TRUE(dbDriver.SaveRecord(1, 1, data, dataLen, "User"));

    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/user/pending/0100000000000000").c_str()));
    FileWrapper f(Path("/db/user/pending/0100000000000000").c_str());
    uint32_t crc = 0;
    ASSERT_EQ(f.Size(), dataLen + sizeof(ObjId) + sizeof(crc));
    ASSERT_TRUE(f.Read(data, dataLen));

    ObjId preCommitId = 0;
    ASSERT_TRUE(f.Read(&preCommitId, sizeof(ObjId)));
    ASSERT_EQ(preCommitId, 1);

    ASSERT_TRUE(f.Read(&crc, sizeof(crc)));
    ASSERT_EQ(crc, crc32(data, dataLen));
}

TEST_F(DbDriverTest, writeScopedUncommittedDataWithPreCommitId) {
    DbDriver dbDriver{1, true};
    const uint32_t dataLen = 10;
    uint8_t data[dataLen] = {0x55};
    ASSERT_TRUE(dbDriver.SaveRecord(1, 10, data, dataLen, "User"));

    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/0100000000000000/user/pending/0100000000000000").c_str()));
    FileWrapper f(Path("/db/0100000000000000/user/pending/0100000000000000").c_str());
    uint32_t crc = 0;
    ASSERT_EQ(f.Size(), dataLen + sizeof(ObjId) + sizeof(crc));
    ASSERT_TRUE(f.Read(data, dataLen));

    ObjId preCommitId = 0;
    ASSERT_TRUE(f.Read(&preCommitId, sizeof(ObjId)));
    ASSERT_EQ(preCommitId, 10);

    ASSERT_TRUE(f.Read(&crc, sizeof(crc)));
    ASSERT_EQ(crc, crc32(data, dataLen));
}

TEST_F(DbDriverTest, dropDb) {
    uint8_t data[10] = {0x55};
    DbDriver dbDriver0{0, false};
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(2, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(3, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver0.SaveRecord(2, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver0.SaveRecord(3, 0, data, sizeof(data), "Blue"));

    DbDriver dbDriver1{1, false};
    ASSERT_TRUE(dbDriver1.SaveRecord(1, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(2, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(3, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(1, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver1.SaveRecord(2, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver1.SaveRecord(3, 0, data, sizeof(data), "Blue"));

    ASSERT_TRUE(DbDriver::DeleteAll());

    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/red/0100000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/red/0200000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/red/0300000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/blue/0100000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/blue/0200000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/blue/0300000000000000").c_str()));

    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0100000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0200000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0300000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0100000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0200000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0300000000000000").c_str()));
}

TEST_F(DbDriverTest, dropTable) {
    uint8_t data[10] = {0x55};
    DbDriver dbDriver0{0, 0};
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(2, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(3, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver0.SaveRecord(2, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver0.SaveRecord(3, 0, data, sizeof(data), "Blue"));

    ASSERT_TRUE(dbDriver0.DeleteTable("Red"));

    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/red/0100000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/red/0200000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/red/0300000000000000").c_str()));

    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0100000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0200000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0300000000000000").c_str()));
}

TEST_F(DbDriverTest, dropScopedDb) {
    uint8_t data[10] = {0x55};
    DbDriver dbDriver0{0, 0};
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(2, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(3, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver0.SaveRecord(2, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver0.SaveRecord(3, 0, data, sizeof(data), "Blue"));

    DbDriver dbDriver1{1, 0};
    ASSERT_TRUE(dbDriver1.SaveRecord(1, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(2, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(3, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(1, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver1.SaveRecord(2, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver1.SaveRecord(3, 0, data, sizeof(data), "Blue"));

    ASSERT_TRUE(dbDriver1.DeleteScope());

    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/red/0100000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/red/0200000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/red/0300000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0100000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0200000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0300000000000000").c_str()));

    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0100000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0200000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0300000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0100000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0200000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0300000000000000").c_str()));
}

TEST_F(DbDriverTest, dropTableScoped) {
    uint8_t data[10] = {0x55};
    DbDriver dbDriver0{0, 0};
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(2, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(3, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver0.SaveRecord(2, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver0.SaveRecord(3, 0, data, sizeof(data), "Blue"));

    DbDriver dbDriver1{1, 0};
    ASSERT_TRUE(dbDriver1.SaveRecord(1, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(2, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(3, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver1.SaveRecord(1, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver1.SaveRecord(2, 0, data, sizeof(data), "Blue"));
    ASSERT_TRUE(dbDriver1.SaveRecord(3, 0, data, sizeof(data), "Blue"));

    ASSERT_TRUE(dbDriver1.DeleteTable("Blue"));

    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/red/0100000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/red/0200000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/red/0300000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0100000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0200000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/blue/0300000000000000").c_str()));

    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0100000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0200000000000000").c_str()));
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/0100000000000000/red/0300000000000000").c_str()));

    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0100000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0200000000000000").c_str()));
    EXPECT_FALSE(DirectoryWrapper::Exists(Path("/db/0100000000000000/blue/0300000000000000").c_str()));
}

TEST_F(DbDriverTest, notificationCallbackTest) {
    bool called = false;
    DbDriver::SetOnCreateCallback([&called](const void * data, size_t len, uint64_t scope, const char* tableName){
        called = true;
        EXPECT_EQ(TestUser::MaxSerializedLength, len - sizeof(ObjId));
        EXPECT_STREQ(tableName, "TestUser");
        EXPECT_EQ(scope, 123);
        ObjId commitId = *(ObjId*)((char*)data + TestUser::MaxSerializedLength);
        EXPECT_EQ(commitId, 321);
        TestUser u;
        u.Deserialize((char*)data, false);
        EXPECT_EQ(u.Name(), "Tester");
        EXPECT_EQ(u.OtherUserId(), 555);

        return true;
    });

    TestUser u;
    u.OtherUserId(555);
    u.Name("Tester");
    Table<TestUser> table {123, true};
    table.Save(u, 321);
    DbError result = table.Commit(u, 321);
    ASSERT_EQ(ErrorCode::None, (ErrorCode)result);
    ASSERT_TRUE(called);
}

TEST_F(DbDriverTest, notificationCallbackDelete) {
    bool called = false;
    DbDriver::SetOnDeleteCallback([&called](const void * data, size_t len, uint64_t scope, const char* tableName) {
        (void) data;
        called = true;
        EXPECT_EQ(len, 18); // data size + id 8 uint_64_t
        EXPECT_EQ(tableName, "Red");
        EXPECT_EQ(scope, 123);
        return true;
    });

    uint8_t data[10] = {0x55, 0x66, 0x88, 0};
    DbDriver dbDriver0{123, 0};
    ASSERT_TRUE(dbDriver0.SaveRecord(1, 0, data, sizeof(data), "Red"));
    ASSERT_TRUE(dbDriver0.DeleteRecord(1, "Red"));
    ASSERT_TRUE(called);
}
