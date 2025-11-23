#include <gtest/gtest.h>
#include <vector>
#include "DbDriver.hpp"
#include "Table.hpp"
#include "TestUser.hpp"
#include "TestUserValidator.hpp"
#include "DbTestObject.hpp"
#include "DbNestTestObject.hpp"
#include "TestUserHelper.hpp"
#include "fs.hpp"

using User = TestUser;
using UserValidator = TestUserValidator;

using namespace base_message;
class EmptyTableTest : public ::testing::Test {
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
            DbDriver::SetOnCreateCallback(nullptr);
            DbDriver::ClearCache();
        }

        void CreateUser(User& u, ObjId scope = DbDriver::RootScope) {
            Table<User> t{scope};
            DbError error = t.Save(u);
            ASSERT_FALSE(error);
        }

        uint16_t gokuRoles = USER_ROLE_ACCOUNT_APPROVER | USER_ROLE_CRYPTO_OFFICER | USER_ROLE_ACCOUNT_MANAGER;
        Table<User> uTable;
        uint8_t pk[User::PublicKeyMaxLength] = {0xD5};
};

class TableTest : public EmptyTableTest {
    protected:
        virtual void SetUp() override {
            if (DirectoryWrapper::Exists(Path("/db").c_str())) {
                DirectoryWrapper::Delete(Path("/db").c_str());
            }
            initFS();
            DbDriver::InitDb();
            DbDriver::ClearCache();
            User u1;
            u1.Name().Set("Goku");
            u1.PublicKey().Set(pk, sizeof(pk));
            char fb1[] = "abc";
            u1.FbToken().Set(fb1);
            u1.CNonce(3);
            u1.Roles(gokuRoles);

            CreateUser(u1, scope);

            User u2;
            u2.Name().Set("Krillin");
            uint8_t otherPk[User::PublicKeyMaxLength] = {0x88};
            u2.PublicKey().Set(otherPk, sizeof(otherPk));
            char fb2[] = "def";
            u2.FbToken().Set(fb2);
            u2.CNonce(4);
            u2.Roles(USER_ROLE_MAINTENANCE | USER_ROLE_ORGANIZATION_MANAGER | USER_ROLE_ACCOUNT_MANAGER);
            CreateUser(u2, scope);

            User u3;
            u3.Name().Set("Piccolo");
            u3.PublicKey().Set(otherPk, sizeof(otherPk));
            char fb3[] = "ghi";
            u3.FbToken().Set(fb3);
            CreateUser(u3, scope);
        }

        ObjId scope = 0;
};

class PendingTableTest : public TableTest {
    protected:
        ObjId commitId = 48;
        virtual void SetUp() override {
            scope = 1;
            TableTest::SetUp();
            Table<User> uncommittedUTable {scope, true};
            User u;
            u.Name("Vegeta");
            uncommittedUTable.Save(u, commitId);

            User u2;
            u2.Name("Beerus");
            uncommittedUTable.Save(u2, commitId + 2);
        }
};

class FullTableTest : public EmptyTableTest {
    protected:
        virtual void SetUp() override {
            initFS();
            DbDriver::InitDb();
            DbDriver::ClearCache();

            for (int i = 0; i < totalRecords; i++) {
                User* u = new User();
                u->PublicKey().Set(pk, sizeof(pk));
                u->Name().Set(std::to_string(i).c_str());
                if (i <= lastAuditor) {
                    u->Roles(USER_ROLE_ACCOUNT_AUDITOR | USER_ROLE_MAINTENANCE | USER_ROLE_ACCOUNT_MANAGER);
                } else {
                    u->Roles(USER_ROLE_CRYPTO_OFFICER | USER_ROLE_MAINTENANCE | USER_ROLE_ACCOUNT_MANAGER);
                }
                CreateUser(*u);
                users.push_back(u);
            }
        }

        virtual void TearDown() override {
            users.clear();
            DirectoryWrapper::Delete(Path("/db").c_str());
            DbDriver::ClearCache();
        }

        const int totalRecords = 350;
        const int lastAuditor = 300;
        std::vector<User*> users;
};

class TwoThingsTest : public TableTest {
    public:
        virtual void SetUp() override {
            initFS();
            DbDriver::InitDb();
            DbDriver::ClearCache();
            for (int i = 0; i < totalUsers; i++) {
                User u;
                u.Name().Set(std::to_string(i).c_str());
                CreateUser(u);
                DbTestObject obj;
                obj.UserId(u.Id());
                testObjTable.Save(obj);
            }
        }

        virtual void TearDown() override {
            DirectoryWrapper::Delete(Path("/db").c_str());
            DbDriver::ClearCache();
        }

        Table<DbTestObject> testObjTable;
        const int totalUsers = 280;
};

class NestedObjTest : public TableTest {
    public:
        virtual void SetUp() override {
            initFS();
            DbDriver::InitDb();
            DbDriver::ClearCache();
        }

        virtual void TearDown() override {
            DirectoryWrapper::Delete(Path("/db").c_str());
            DbDriver::ClearCache();
        }

        uint64_t SaveAnObj() {
            DbNestTestObject obj;
            obj.DbTest().UserId(7);
            obj.TestDec().DecisionType(Decision::DecisionType_t::NextRule);
            EXPECT_EQ(testObjTable.Save(obj), ErrorCode::None);
            return obj.Id();
        }

        Table<DbNestTestObject> testObjTable;
};

class ScopedTablesTest : public EmptyTableTest {
    protected:
        virtual void SetUp() override {
            initFS();
            DbDriver::InitDb();
            DbDriver::ClearCache();

            for (int i = 0; i < totalRecords; i++) {
                User* u = new User();
                u->PublicKey().Set(pk, sizeof(pk));
                u->Name().Set(("John" + std::to_string(i)).c_str());
                CreateUser(*u);
                users0.push_back(u);
            }

            for (int i = 0; i < totalRecords; i++) {
                User* u = new User();
                u->PublicKey().Set(pk, sizeof(pk));
                u->Name().Set(("Steve" + std::to_string(i)).c_str());
                CreateUser(*u, 1);
                users1.push_back(u);
            }
        }

        virtual void TearDown() override {
            users0.clear();
            users1.clear();
            DirectoryWrapper::Delete(Path("/db").c_str());
            DbDriver::ClearCache();
        }

        const int totalRecords = 350;
        std::vector<User*> users0;
        std::vector<User*> users1;
};

TEST_F(EmptyTableTest, saveANewRecord) {
    User u;
    u.Name().Set("Goku");
    CreateUser(u);
    EXPECT_TRUE(DirectoryWrapper::Exists(Path("/db/testuser/0100000000000000").c_str()));
}

TEST_F(TableTest, findARecord) {
    ASSERT_TRUE(uTable.Find(1));
    User& u = uTable.LoadedRecord();
    EXPECT_TRUE(0 == strcmp(u.Name(), "Goku"));
    EXPECT_TRUE(0 == strcmp(u.FbToken(), "abc"));
    EXPECT_TRUE(0 == memcmp(u.PublicKey().buffer, pk, sizeof(pk)));
    EXPECT_EQ(3, u.CNonce());
    EXPECT_EQ(gokuRoles, u.Roles());
    EXPECT_EQ(1, u.Id());
}

TEST_F(TableTest, updateARecord) {
    ASSERT_TRUE(uTable.Find(1));
    uTable.LoadedRecord().Name().Set("Vegeta");
    ASSERT_EQ(uTable.Save(uTable.LoadedRecord()), ErrorCode::None);
    ASSERT_TRUE(uTable.Find(1));
    EXPECT_TRUE(0 == strcmp(uTable.LoadedRecord().Name(), "Vegeta"));
}

TEST_F(TableTest, updateARecordDoesNotSendDbEvent) {
    DbDriver::SetOnCreateCallback([](const void * data, size_t len, uint64_t scope, const char* tableName){
        (void)data;
        (void)len;
        (void)scope;
        (void)tableName;
        EXPECT_TRUE(false); //this should NOT be called. on an update we dont send a db event.
        return true;
    });
    ASSERT_TRUE(uTable.Find(1));
    uTable.LoadedRecord().Name().Set("Vegeta");
    ASSERT_EQ(uTable.Save(uTable.LoadedRecord()), ErrorCode::None);
    ASSERT_TRUE(uTable.Find(1));
    EXPECT_TRUE(0 == strcmp(uTable.LoadedRecord().Name(), "Vegeta"));
}

TEST_F(TableTest, canPeekNextId) {
    ObjId nextId = uTable.PeekNextId();
    ASSERT_EQ(4, nextId);
    nextId = uTable.PeekNextId();
    ASSERT_EQ(4, nextId);
}

TEST_F(TableTest, canUsePeekedId) {
    ObjId nextId = uTable.PeekNextId();
    User u;
    u.Name("Vegeta");
    u.Id(nextId);
    CreateUser(u);
    ASSERT_EQ(u.Id(), nextId);

    nextId = uTable.PeekNextId();
    ASSERT_NE(u.Id(), nextId);
}

TEST_F(TableTest, updatingARecordDoesntIncrementIdCounter) {
    ObjId nextId = uTable.PeekNextId();
    ASSERT_TRUE(uTable.Find(3));
    uTable.LoadedRecord().Name().Set("Vegeta");
    ASSERT_EQ(uTable.Save(uTable.LoadedRecord()), ErrorCode::None);
    ObjId nextIdAfterUpdate = uTable.PeekNextId();
    ASSERT_EQ(nextId, nextIdAfterUpdate);
}

TEST_F(TableTest, findByAProperty) {
    char name[] = "Goku";
    User& u = uTable.LoadedRecord();
    ASSERT_TRUE(uTable.FindBy("Name", name));
    EXPECT_TRUE(0 == strcmp(u.Name(), "Goku"));
    EXPECT_EQ(1, u.Id());

    uint64_t nonce = 3;
    ASSERT_TRUE(uTable.FindBy("CNonce", &nonce, 8));
    EXPECT_TRUE(0 == strcmp(u.Name(), "Goku"));
    EXPECT_EQ(1, u.Id());

    ASSERT_TRUE(uTable.FindBy("PublicKey", pk, sizeof(pk)));
    EXPECT_TRUE(0 == strcmp(u.Name(), "Goku"));
    EXPECT_EQ(1, u.Id());

    *name = 'R';
    ASSERT_FALSE(uTable.FindBy("Name", name));
}

TEST_F(TableTest, findByStart) {
    char name[] = "Kr";
    User& u = uTable.LoadedRecord();
    ASSERT_TRUE(uTable.FindBy("Name", name, false));
    EXPECT_TRUE(0 == strcmp(u.Name(), "Krillin"));
    EXPECT_EQ(2, u.Id());
}

TEST_F(TableTest, findByMask) {
    User& u = uTable.LoadedRecord();
    ASSERT_TRUE(uTable.FindByMask("Roles", USER_ROLE_MAINTENANCE));
    EXPECT_TRUE(0 == strcmp(u.Name(), "Krillin"));
    EXPECT_TRUE(test_user::isMantainance(u));
}

TEST_F(TableTest, deleteRecord) {
    uTable.Delete(1);
    EXPECT_FALSE(uTable.Find(1));
    User& u = uTable.LoadedRecord();
    ASSERT_TRUE(uTable.Find(2));
    ASSERT_EQ(ErrorCode::None, uTable.Delete(u));
    EXPECT_FALSE(uTable.Find(2));
}

TEST_F(TableTest, deleteResults) {
    auto results = uTable.CustomSearch(Query::ResultType::Many, [](User* u) { return u->Id() > 1; });
    ASSERT_FALSE(uTable.Delete(results));

    EXPECT_TRUE(uTable.Find(1));
    EXPECT_FALSE(uTable.Find(2));
    EXPECT_FALSE(uTable.Find(3));
}

TEST_F(TableTest, dropTable) {
    ASSERT_TRUE(uTable.DropTable());
    EXPECT_FALSE(uTable.Find(1));
    EXPECT_FALSE(uTable.Find(2));
    User u;
    u.Name().Set("Bulma");
    EXPECT_EQ(uTable.Save(u), ErrorCode::None);
    EXPECT_NE(u.Id(), 0);
}

TEST_F(TableTest, customQuery) {
    User& u = uTable.LoadedRecord();
    auto results = uTable.CustomSearch(Query::ResultType::Single, [](User* u) {
        return u->Roles() & USER_ROLE_ACCOUNT_MANAGER && u->Roles() & USER_ROLE_MAINTENANCE ;
    });
    ASSERT_TRUE(results.success);
    EXPECT_TRUE(0 == strcmp(u.Name(), "Krillin"));
}

TEST_F(TableTest, customCountQuery) {
    auto results = uTable.CustomSearch(Query::ResultType::Count, [](User* u) {
        return u->Roles() & USER_ROLE_ACCOUNT_MANAGER && u->Roles() & USER_ROLE_MAINTENANCE ;
    });
    ASSERT_TRUE(results.success);
    EXPECT_EQ(results.GetCount(), 1);
}

TEST_F(TableTest, CustomSearchForManyCanDestructivelyAlterWorkBuffer) {
    auto results1 = uTable.CustomSearch(Query::ResultType::Many, [](auto ...) {
        return true;
    });
    std::set<ObjId> userSet1;
    while(uTable.LoadNextResult(results1)) {
        userSet1.insert(uTable.LoadedRecord().Id());
    }

    auto results2 = uTable.CustomSearch(Query::ResultType::Many, [](auto ...) {
        memset(DbDriver::WorkBuffer(), 0, 10);
        return true;
    });
    std::set<ObjId> userSet2;
    while(uTable.LoadNextResult(results2)) {
        userSet2.insert(uTable.LoadedRecord().Id());
    }
    ASSERT_TRUE(userSet1 == userSet2);
}

TEST_F(TableTest, CustomSearchForSingleCanDestructivelyAlterWorkBuffer) {
    auto results1 = uTable.CustomSearch(Query::ResultType::Single, [](auto ...) {
        return true;
    });
    ASSERT_TRUE(results1.success);
    User user1 = uTable.LoadedRecord();

    auto results2 = uTable.CustomSearch(Query::ResultType::Single, [](auto ...) {
        memset(DbDriver::WorkBuffer(), 0, 10);
        return true;
    });
    ASSERT_TRUE(results2.success);
    ASSERT_TRUE(user1 == uTable.LoadedRecord());
    ASSERT_NE(0, uTable.LoadedRecord().Id());
}

TEST_F(TableTest, CustomSearchForCountCanDestructivelyAlterWorkBuffer) {
    auto results1 = uTable.CustomSearch(Query::ResultType::Count, [](auto ...) {
        return true;
    });
    ASSERT_TRUE(results1.success);

    auto results2 = uTable.CustomSearch(Query::ResultType::Count, [](auto ...) {
        memset(DbDriver::WorkBuffer(), 0, 10);
        return true;
    });
    ASSERT_TRUE(results2.success);
    ASSERT_TRUE(results1.GetCount() == results2.GetCount());
}

TEST_F(FullTableTest, wherePropertyMatches)
{
    const char* name = {"100"};
    ResultSet<User> results = uTable.Where("Name", name);
    ASSERT_TRUE(results.success);
    ASSERT_TRUE(uTable.LoadNextResult(results));
    User& u = uTable.LoadedRecord();
    EXPECT_TRUE(0 == strcmp(u.Name(), "100"));
    EXPECT_FALSE(uTable.LoadNextResult(results));
}

TEST_F(FullTableTest, wherePropertyKindaMatches)
{
    const char* name = {"10"};
    ResultSet<User> results = uTable.Where("Name", name, false);
    ASSERT_TRUE(results.success);

    User& u = uTable.LoadedRecord();

    std::vector<std::string> names = {"10"};
    for (int i = 100; i < 110; i++) {
        names.push_back(std::to_string(i).c_str());
    }

    for (size_t i = 0; i < names.size(); i++) {
        ASSERT_TRUE(uTable.LoadNextResult(results));
        EXPECT_TRUE(std::find(names.begin(), names.end(), std::string(u.Name())) != names.end());
    }

    EXPECT_FALSE(uTable.LoadNextResult(results));
}

TEST_F(FullTableTest, multiplePageResult)
{
    ResultSet<User> results = uTable.Where("PublicKey", pk, sizeof(pk));
    ASSERT_TRUE(results.success);

    User& u = uTable.LoadedRecord();

    std::vector<uint64_t> ids;
    for (int i = 1; i <= totalRecords; i++) {
        ids.push_back(i);
    }

    std::vector<uint64_t> found;
    for (int i = 0; i < totalRecords; i++) {
        ASSERT_TRUE(uTable.LoadNextResult(results));
        found.push_back(u.Id());
        auto id = std::find(ids.begin(), ids.end(), u.Id());
        ASSERT_NE(id, ids.end());
        ids.erase(id);
    }

    EXPECT_EQ(found.size(), totalRecords);
    EXPECT_EQ(ids.size(), 0);

    ASSERT_FALSE(uTable.LoadNextResult(results));
}

TEST_F(FullTableTest, whereMask)
{
    ResultSet<User> results = uTable.WhereMask("Roles", USER_ROLE_ACCOUNT_AUDITOR);
    ASSERT_TRUE(results.success);

    User& u = uTable.LoadedRecord();

    std::vector<uint64_t> ids;
    for (int i = 1; i <= lastAuditor + 1; i++) {
        ids.push_back(i);
    }

    std::vector<uint64_t> found;
    for (int i = 0; i <= lastAuditor; i++) {
        ASSERT_TRUE(uTable.LoadNextResult(results));
        found.push_back(u.Id());
        auto id = std::find(ids.begin(), ids.end(), u.Id());
        ASSERT_NE(id, ids.end());
        ids.erase(id);
    }

    EXPECT_EQ(found.size(), lastAuditor + 1);
    EXPECT_EQ(ids.size(), 0);

    ASSERT_FALSE(uTable.LoadNextResult(results));
}

TEST_F(FullTableTest, fetchAll)
{
    ResultSet<User> results = uTable.All();
    ASSERT_TRUE(results.success);

    User& u = uTable.LoadedRecord();

    std::vector<uint64_t> ids;
    for (int i = 1; i <= totalRecords; i++) {
        ids.push_back(i);
    }

    std::vector<uint64_t> found;
    for (int i = 0; i < totalRecords; i++) {
        ASSERT_TRUE(uTable.LoadNextResult(results));
        found.push_back(u.Id());
        auto id = std::find(ids.begin(), ids.end(), u.Id());
        ASSERT_NE(id, ids.end());
        ids.erase(id);
    }

    EXPECT_EQ(found.size(), totalRecords);
    EXPECT_EQ(ids.size(), 0);
    ASSERT_FALSE(uTable.LoadNextResult(results));
}
TEST_F(FullTableTest, countWhere)
{
    const char * name = {"10"};
    ResultSet<User> results = uTable.Count("Name", name, false);
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 11);
}

TEST_F(FullTableTest, countWhereDoesntMatchAnything)
{
    const char * name = {"100000000"};
    ResultSet<User> results = uTable.Count("Name", name);
    ASSERT_FALSE(results.success);
    ASSERT_EQ(results.GetCount(), 0);
}

TEST_F(FullTableTest, countAll)
{
    ResultSet<User> results = uTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), totalRecords);
}

TEST_F(FullTableTest, countMask)
{
    ResultSet<User> results = uTable.CountMask("Roles", USER_ROLE_ACCOUNT_AUDITOR);
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), lastAuditor + 1);
}

TEST_F(FullTableTest, countWhereNot)
{
    const char * name = {"10"};
    ResultSet<User> results = uTable.Not().Count("Name", name, false);
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), totalRecords - 11);
}

TEST_F(FullTableTest, countMaskNot)
{
    ResultSet<User> results = uTable.Not().CountMask("Roles", USER_ROLE_ACCOUNT_AUDITOR);
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), totalRecords - lastAuditor - 1);
}

TEST_F(FullTableTest, findWhereNot)
{
    const char * name = {"0"};
    User& u = uTable.LoadedRecord();
    ASSERT_TRUE(uTable.Not().FindBy("Name", name));
    EXPECT_NE(u.Id(), 1);
}

TEST_F(FullTableTest, findMaskNot)
{
    User& u = uTable.LoadedRecord();
    ASSERT_TRUE(uTable.Not().FindByMask("Roles", USER_ROLE_ACCOUNT_AUDITOR));
    ASSERT_FALSE(u.Roles() & USER_ROLE_ACCOUNT_AUDITOR);
}

TEST_F(FullTableTest, whereNot)
{
    const char* name = {"1"};
    ResultSet<User> results = uTable.Not().Where("Name", name, false);
    ASSERT_TRUE(results.success);
    ASSERT_TRUE(uTable.LoadNextResult(results));
    User& u = uTable.LoadedRecord();
    int count = 0;
    while(uTable.LoadNextResult(results)) {
        count++;
        EXPECT_NE(u.Name()[0], '1');
    }
    EXPECT_EQ(count, totalRecords - 100 - 10 - 1 - 1);
}

TEST_F(FullTableTest, whereMaskNot)
{
    User& u = uTable.LoadedRecord();
    ResultSet<User> results = uTable.Not().WhereMask("Roles", USER_ROLE_ACCOUNT_AUDITOR);
    ASSERT_TRUE(results.success);

    int count = 0;
    while(uTable.LoadNextResult(results)) {
        count++;
        EXPECT_FALSE(u.Roles() & USER_ROLE_ACCOUNT_AUDITOR);
    }
    EXPECT_EQ(count, totalRecords - lastAuditor - 1);
}

TEST_F(FullTableTest, customSearch)
{
    auto test = [](User* u) {
        return u->Roles() & USER_ROLE_ACCOUNT_MANAGER && u->Roles() & USER_ROLE_MAINTENANCE ;
    };
    auto results = uTable.CustomSearch(Query::ResultType::Many, test);
    ASSERT_TRUE(results.success);

    User& u = uTable.LoadedRecord();

    for (int i = 0; i <= totalRecords; i++) {
        if (i < totalRecords) {
            ASSERT_TRUE(uTable.LoadNextResult(results));
            ASSERT_TRUE(test(&u));
        } else {
            ASSERT_FALSE(uTable.LoadNextResult(results));
        }
    }

}

TEST_F(TwoThingsTest, canQueryTwoThingsAtOnce)
{
    DbTestObject& obj = testObjTable.LoadedRecord();
    auto userResults = uTable.All();
    ASSERT_TRUE(userResults.success);
    uint64_t i = 1;
    while (uTable.LoadNextResult(userResults)) {
        i++;
        auto testObjResults = testObjTable.Not().Where("UserId", &i, sizeof(i));
        ASSERT_TRUE(testObjResults.success);

        while (testObjTable.LoadNextResult(testObjResults)) {
            ASSERT_NE(obj.UserId(), i);
        }
    }
    ASSERT_EQ(i, totalUsers + 1);
}

TEST_F(NestedObjTest, canSaveNestedObj)
{
    uint64_t id = SaveAnObj();
    ASSERT_TRUE(testObjTable.Find(id));
    ASSERT_EQ(testObjTable.LoadedRecord().DbTest().UserId(), 7);
}

TEST_F(NestedObjTest, findByNestedProperty)
{
    SaveAnObj();
    ASSERT_TRUE(testObjTable.FindBy("TestDec", "NextRule", strlen("NextRule") + 1));
    ASSERT_EQ(testObjTable.LoadedRecord().TestDec().DecisionType(), Decision::DecisionType_t::NextRule);
}

TEST_F(ScopedTablesTest, canQueryScopedData)
{
    Table<User> scopedTable{1};

    ResultSet<User> results = scopedTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), totalRecords);

    results = scopedTable.Count("Name", "John", false);
    ASSERT_FALSE(results.success);
    ASSERT_EQ(results.GetCount(), 0);

    results = scopedTable.Count("Name", "Steve", false);
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), totalRecords);
}

TEST_F(PendingTableTest, PendingTableDoesntSeeMainTable) {
    Table<User> pendingTable{scope, 1};
    ASSERT_FALSE(pendingTable.FindBy("Name", "Goku"));
    ASSERT_TRUE(pendingTable.FindBy("Name", "Vegeta"));

    auto results = pendingTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 2);
}

TEST_F(PendingTableTest, MainTableDoesntSeePendingRecords) {
    Table<User> table{scope, 0};
    ASSERT_TRUE(table.FindBy("Name", "Goku"));
    ASSERT_FALSE(table.FindBy("Name", "Vegeta"));

    auto results = table.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 3);
}

TEST_F(PendingTableTest, PendingTableSavesDataInPendingState) {
    Table<User> mainTable{scope, false};
    Table<User> pendingTable{scope, true};

    auto results = mainTable.CountAll();
    ASSERT_TRUE(results.success);
    size_t count = results.GetCount();

    User u;
    u.Name("Buu");
    pendingTable.Save(u, 1);

    results = mainTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), count);
}

TEST_F(PendingTableTest, PendingTableCanFindAllByCommitId) {
    Table<User> pendingTable{scope, true};
    User u;
    u.Name("Buu");
    ASSERT_EQ(ErrorCode::None, pendingTable.Save(u, commitId));

    auto result = pendingTable.AllWithCommitId(commitId);
    size_t total = 0;
    while (pendingTable.LoadNextResult(result)) {
        ASSERT_TRUE(
            0 == strcmp(pendingTable.LoadedRecord().Name(), "Vegeta") ||
            0 == strcmp(pendingTable.LoadedRecord().Name(), "Buu")
        );
        ASSERT_EQ(pendingTable.LoadedRecordCommitId(), commitId);
        total++;
    }

    ASSERT_EQ(total, 2);
}

TEST_F(PendingTableTest, PendingTableCanCommitAllByCommitId) {
    Table<User> pendingTable{scope, true};
    User u;
    u.Name("Buu");

    auto err = pendingTable.Save(u, commitId);
    ASSERT_FALSE(err);

    err = pendingTable.CommitAll(commitId);
    ASSERT_EQ(ErrorCode::None, err.Code());

    auto results = pendingTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 1);

    results = pendingTable.AllWithCommitId(commitId);
    ASSERT_FALSE(results.success);

    Table<User> mainTable{scope};
    results = mainTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 5);

    ASSERT_TRUE(mainTable.FindBy("Name", "Vegeta"));
    ASSERT_TRUE(mainTable.FindBy("Name", "Buu"));
}

TEST_F(PendingTableTest, PendingTableCanCommitOneByOne) {
    Table<User> pendingTable{scope, true};
    User u;
    u.Name("Buu");
    pendingTable.Save(u, commitId);

    std::map<std::string, ObjId> created;
    auto results = pendingTable.AllWithCommitId(commitId);
    while (pendingTable.LoadNextResult(results)) {
        auto& user = pendingTable.LoadedRecord();
        ASSERT_EQ(ErrorCode::None, pendingTable.Commit(user, commitId));
        created[std::string(user.Name())] = user.Id();
    }

    results = pendingTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 1);

    results = pendingTable.AllWithCommitId(commitId);
    ASSERT_FALSE(results.success);

    Table<User> mainTable{scope};
    results = mainTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 5);

    ASSERT_TRUE(mainTable.FindBy("Name", "Vegeta"));
    ASSERT_EQ(mainTable.LoadedRecord().Id(), created["Vegeta"]);
    ASSERT_TRUE(mainTable.FindBy("Name", "Buu"));
    ASSERT_EQ(mainTable.LoadedRecord().Id(), created["Buu"]);
}
TEST_F(PendingTableTest, CommitSendsDbEventsForNewRecords) {
    bool called = false;
    DbDriver::SetOnCreateCallback([&called](const void * data, size_t len, uint64_t scope, const char* tableName){
        (void)data;
        called = true;
        EXPECT_EQ(len, TestUser::MaxSerializedLength + sizeof(ObjId));
        EXPECT_STREQ(tableName, "TestUser");
        EXPECT_EQ(scope, 1);
        return true;
    });

    Table<User> pendingTable{scope, true};
    User u;
    u.Name("Buu");
    pendingTable.Save(u, commitId);

    std::map<std::string, ObjId> created;
    auto results = pendingTable.AllWithCommitId(commitId);
    while (pendingTable.LoadNextResult(results)) {
        auto& user = pendingTable.LoadedRecord();
        ASSERT_EQ(ErrorCode::None, pendingTable.Commit(user, commitId));
        created[std::string(user.Name())] = user.Id();
    }
    ASSERT_TRUE(called);
}
TEST_F(PendingTableTest, CommitDoesntSendDbEventsForNewRecordsWhenCallBackFunctionIsNot) {
    Table<User> pendingTable{scope, true};
    User u;
    u.Name("Buu");
    pendingTable.Save(u, commitId);

    std::map<std::string, ObjId> created;
    auto results = pendingTable.AllWithCommitId(commitId);
    while (pendingTable.LoadNextResult(results)) {
        auto& user = pendingTable.LoadedRecord();
        ASSERT_EQ(ErrorCode::None, pendingTable.Commit(user, commitId));
        created[std::string(user.Name())] = user.Id();
    }
}

TEST_F(PendingTableTest, TableWontSavePendingDataWithoutCommitId) {
    Table<User> pendingTable{scope, true};
    User u;
    ASSERT_NE(ErrorCode::None, pendingTable.Save(u));
}

TEST_F(PendingTableTest, PendingTableValidatesOnWrite) {
    Table<User, TestUserValidator> pendingTable{1, true};
    User u;
    ASSERT_NE(ErrorCode::None, pendingTable.Save(u, commitId));
}

TEST_F(PendingTableTest, UniqueValidationsOccurAgainstBothTables) {
    Table<User, UserValidator> pendingTable{scope, true};
    uint8_t cellPk[40] = {0x03, 0x08, 0x82};
    User cell;
    cell.Name("Cell");
    cell.PublicKey(cellPk, sizeof(cellPk));
    cell.Roles(USER_ROLE_CRYPTO_OFFICER);
    ASSERT_EQ(ErrorCode::None, pendingTable.Save(cell, commitId));

    User gokuAgain;
    gokuAgain.Name("Goku");
    gokuAgain.PublicKey(pk, sizeof(pk));
    gokuAgain.Roles(USER_ROLE_CRYPTO_OFFICER);
    ASSERT_EQ(ErrorCode::DbRecordNotUnique, pendingTable.Save(gokuAgain, commitId));

    User cellAgain;
    cellAgain.Name("Cell");
    cellAgain.PublicKey(cellPk, sizeof(cellPk));
    cellAgain.Roles(USER_ROLE_CRYPTO_OFFICER);
    ASSERT_EQ(ErrorCode::DbRecordNotUnique, pendingTable.Save(cellAgain, commitId));
}

TEST_F(PendingTableTest, PendingTableValidatesOnCommit) {
    Table<User> unvalidatedCommit {scope, true};
    User u;
    ASSERT_EQ(ErrorCode::None, unvalidatedCommit.Save(u, 1));

    Table<User, UserValidator> validatedCommit {scope, true};
    ASSERT_NE(ErrorCode::None, validatedCommit.CommitAll(1));
}

TEST_F(PendingTableTest, PendingTableValidatesBeforeExecution) {
    Table<User> unvalidatedCommit {scope, true};
    User u;
    ASSERT_EQ(ErrorCode::None, unvalidatedCommit.Save(u, commitId));

    Table<User, UserValidator> validatedCommit {scope, true};
    ASSERT_NE(ErrorCode::None, validatedCommit.CommitAll(commitId));

    Table<User> mainTable {scope};
    auto results = mainTable.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 3);

    results = validatedCommit.CountAll();
    ASSERT_TRUE(results.success);
    ASSERT_EQ(results.GetCount(), 3);
}

TEST_F(PendingTableTest, CommitCanBeValidated) {
    Table<User> unvalidatedCommit {scope, true};
    User u;
    ASSERT_EQ(ErrorCode::None, unvalidatedCommit.Save(u, commitId));

    Table<User, UserValidator> validatedCommit {scope, true};
    ASSERT_NE(ErrorCode::None, validatedCommit.ValidateCommit(commitId));
}

TEST_F(PendingTableTest, PendingDataCanBeDeleted) {
    Table<User> pendingTable {scope, true};
    User u;
    ASSERT_EQ(ErrorCode::None, pendingTable.Save(u, commitId));
    ASSERT_EQ(ErrorCode::None, pendingTable.Delete(u));
    ASSERT_FALSE(pendingTable.Find(u.Id()));
}

TEST_F(PendingTableTest, CommitCanBeCancelled) {
    Table<User> pendingTable {scope, true};
    User u;
    ASSERT_EQ(ErrorCode::None, pendingTable.Save(u, 1));
    ASSERT_EQ(ErrorCode::None, pendingTable.CancelCommit(1));
    ASSERT_FALSE(pendingTable.AllWithCommitId(1).success);
}
