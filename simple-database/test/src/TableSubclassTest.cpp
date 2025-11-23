#include <gtest/gtest.h>
#include "Table.hpp"
#include "TestUser.hpp"
#include "TestUserValidator.hpp"
#include "fs.hpp"


using User = TestUser;

class SubclassedTable : public Table<User> {
    public:
        bool afterSaveCalled;
        bool afterDeleteCalled;
        bool beforeSaveCalled;
        DbError beforeSaveError;

        void Reset() {
            afterSaveCalled = false;
            afterDeleteCalled = false;
            beforeSaveCalled = false;
            beforeSaveError = ErrorCode::None;
        }

        SubclassedTable(bool pending = false) : Table<User>{0, pending} {
            Reset();
        }

        virtual void AfterSave(User&) override {
            afterSaveCalled = true;
        }

        virtual DbError BeforeSave(User&) override {
            beforeSaveCalled = true;
            return beforeSaveError;
        }

        virtual void AfterDelete(ObjId, User&) override {
            afterDeleteCalled = true;
        }
};

class TableSubclassTest : public ::testing::Test {
    protected:
        virtual void SetUp() {
            if (DirectoryWrapper::Exists(Path("/db").c_str())) {
                DirectoryWrapper::Delete(Path("/db").c_str());
            }
            initFS();
            DbDriver::InitDb();
            DbDriver::ClearCache();
        }
        virtual void TearDown() {
            DirectoryWrapper::Delete(Path("/db").c_str());
            DbDriver::ClearCache();
        }
};

TEST_F(TableSubclassTest, BeforeSaveCalled) {
    User u;
    SubclassedTable t;
    ASSERT_EQ(ErrorCode::None, t.Save(u));
    ASSERT_TRUE(t.beforeSaveCalled);
}

TEST_F(TableSubclassTest, BeforeSaveCanPreventSavingWhenErrorOccurs) {
    User u;
    SubclassedTable t;
    t.beforeSaveError = ErrorCode::General;
    ASSERT_EQ(ErrorCode::General, t.Save(u));
    ASSERT_TRUE(t.beforeSaveCalled);
    ASSERT_FALSE(t.afterSaveCalled);
}

TEST_F(TableSubclassTest, AfterSaveCalled) {
    User u;
    SubclassedTable t;
    ASSERT_EQ(ErrorCode::None, t.Save(u));
    ASSERT_TRUE(t.afterSaveCalled);
}

TEST_F(TableSubclassTest, AfterDeleteCalled) {
    User u;
    Table<User> ut;
    ASSERT_EQ(ErrorCode::None, ut.Save(u));

    SubclassedTable t;
    ASSERT_EQ(ErrorCode::None, t.Delete(u));
    ASSERT_FALSE(t.beforeSaveCalled);
    ASSERT_FALSE(t.afterSaveCalled);
    ASSERT_TRUE(t.afterDeleteCalled);
}

TEST_F(TableSubclassTest, PreCommitBeforeSaveNotCalled) {
    User u;
    SubclassedTable t {true};
    ASSERT_EQ(ErrorCode::None, t.Save(u, 10));
    ASSERT_FALSE(t.beforeSaveCalled);
}

TEST_F(TableSubclassTest, PreCommitAfterSaveNotCalled) {
    User u;
    SubclassedTable t{true};
    ASSERT_EQ(ErrorCode::None, t.Save(u, 10));
    ASSERT_FALSE(t.afterSaveCalled);
}

TEST_F(TableSubclassTest, PreCommitAfterDeleteNotCalled) {
    User u;
    Table<User> ut {0, true};
    ASSERT_EQ(ErrorCode::None, ut.Save(u, 10));

    SubclassedTable t {true};
    ASSERT_EQ(ErrorCode::None, t.Delete(u));
    ASSERT_FALSE(t.afterDeleteCalled);
}

TEST_F(TableSubclassTest, AfterDeleteisGettingCalledWithDelete)
{
    User u;
    Table<User> ut {0, true};
    ASSERT_EQ(ErrorCode::None, ut.Save(u, 10));

    SubclassedTable t {true};
    ASSERT_EQ(t.Delete(u, true), ErrorCode::None);
    ASSERT_TRUE(t.afterDeleteCalled);
}

TEST_F(TableSubclassTest, AfterDeleteGettingCalledWithCancelCommit)
{
    User u;
    Table<User> ut {0, true};
    ASSERT_EQ(ErrorCode::None, ut.Save(u, 10));

    SubclassedTable t {true};
    ASSERT_EQ(t.CancelCommit(10), ErrorCode::None);
    ASSERT_TRUE(t.afterDeleteCalled);
}

TEST_F(TableSubclassTest, CommitingAfterSaving)
{
    User u;
    Table<User> ut {0, true};
    ASSERT_EQ(ErrorCode::None, ut.Save(u, 10));

    SubclassedTable t {true};
    ASSERT_EQ(ErrorCode::None, t.Commit(u, 10));
}
