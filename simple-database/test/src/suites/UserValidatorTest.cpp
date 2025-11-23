#include <gtest/gtest.h>
#include "Table.hpp"
#include "TestUser.hpp"
#include "TestUserValidator.hpp"
#include "TestUserHelper.hpp"
#include "fs.hpp"

typedef TestUser User;
typedef TestUserValidator UserValidator;

class UserValidatorTest : public ::testing::Test {
    protected:
        virtual void SetUp() {
            initFS();
            DbDriver::InitDb();
            Table<User> table;
            otherUser.PublicKey(otherUserPk, sizeof(otherUserPk));
            table.Save(otherUser);
        }
        virtual void TearDown() {
            otherUser.Reset();
            DbDriver::DeleteAll();
        }
        Table<User, UserValidator> uTable{DbDriver::RootScope};
        User otherUser;
        uint8_t otherUserPk[7] = {1, 2, 3, 4, 5, 6};
};

TEST_F(UserValidatorTest, publicKeyCantBeEmpty) {
    User u;
    u.Name().Set("Charlie Day");
    u.Roles(USER_ROLE_ACCOUNT_APPROVER);
    char fb[] = "abc";
    u.FbToken().Set(fb);
    u.CNonce(3);
    u.OtherUserId(otherUser.Id());
    auto err = uTable.Save(u);
    ASSERT_EQ(err, ErrorCode::DbNotEmptyValidation);
    ASSERT_EQ(err.Details(), "Public key can't be empty");
    uint8_t pk[User::PublicKeyMaxLength] = {0};
    u.PublicKey().Set(pk, sizeof(pk));
    ASSERT_EQ(uTable.Save(u), ErrorCode::None);
};

TEST_F(UserValidatorTest, nameCantBeEmpty) {
    User u;
    u.Roles(USER_ROLE_ACCOUNT_APPROVER);
    char fb[] = "abc";
    u.FbToken().Set(fb);
    u.CNonce(3);
    u.OtherUserId(otherUser.Id());
    uint8_t pk[User::PublicKeyMaxLength];
    memset(pk, 0xdb, sizeof(pk));
    u.PublicKey().Set(pk, sizeof(pk));
    auto err = uTable.Save(u);
    ASSERT_EQ(err, ErrorCode::DbNotEmptyValidation);
    ASSERT_EQ(err.Details(), "Name can't be empty");
    u.Name().Set("Charlie Day");
    ASSERT_EQ(uTable.Save(u), ErrorCode::None);
};

TEST_F(UserValidatorTest, mustHaveARole) {
    User u;
    u.Name().Set("Charlie Day");
    char fb[] = "abc";
    u.FbToken().Set(fb);
    u.CNonce(3);
    u.OtherUserId(otherUser.Id());
    uint8_t pk[User::PublicKeyMaxLength];
    memset(pk, 0x95, sizeof(pk));
    u.PublicKey().Set(pk, sizeof(pk));
    auto err = uTable.Save(u);
    ASSERT_EQ(err, ErrorCode::DbNotZeroValidation);
    ASSERT_EQ(err.Details(), "User must have at least one role");
    u.Roles(USER_ROLE_ACCOUNT_APPROVER);
    ASSERT_EQ(uTable.Save(u), ErrorCode::None);
};

TEST_F(UserValidatorTest, publicKeyMustBeUnique) {
    User u;
    u.Name().Set("Charlie Day");
    u.Roles(USER_ROLE_ACCOUNT_APPROVER);
    char fb[] = "abc";
    u.FbToken().Set(fb);
    u.CNonce(3);
    u.OtherUserId(otherUser.Id());
    uint8_t pk[User::PublicKeyMaxLength - 10];
    memset(pk, 0x87, sizeof(pk));
    u.PublicKey().Set(pk, sizeof(pk));
    ASSERT_EQ(uTable.Save(u), ErrorCode::None);

    User u2;
    u2.Name().Set("Ronald McDonald");
    u2.Roles(USER_ROLE_ACCOUNT_APPROVER);
    u2.FbToken().Set(fb);
    u2.CNonce(3);
    u.OtherUserId(otherUser.Id());
    u2.PublicKey().Set(pk, sizeof(pk));
    auto err = uTable.Save(u2);
    ASSERT_EQ(err, ErrorCode::DbRecordNotUnique);
    ASSERT_EQ(err.Details(), "Public key must be unique");

    uint8_t pk2[User::PublicKeyMaxLength - 14];
    memset(pk2, 6, sizeof(pk2));
    u2.PublicKey().Set(pk2, sizeof(pk2));
    ASSERT_EQ(uTable.Save(u2), ErrorCode::None);
};

TEST_F(UserValidatorTest, foreignKeyMustBeValid) {
    User u;
    u.Name().Set("Charlie Day");
    u.Roles(USER_ROLE_ACCOUNT_APPROVER);
    char fb[] = "abc";
    u.FbToken().Set(fb);
    u.CNonce(3);
    u.OtherUserId(otherUser.Id() + 1);
    uint8_t pk[User::PublicKeyMaxLength];
    memset(pk, 0x95, sizeof(pk));
    u.PublicKey().Set(pk, sizeof(pk));
    auto err = uTable.Save(u);
    ASSERT_EQ(err, ErrorCode::DbForeignKeyNotValid);
    ASSERT_EQ(err.Details(), "Other user does not exist");
    u.OtherUserId(otherUser.Id());
    ASSERT_EQ(uTable.Save(u), ErrorCode::None);
};

TEST_F(UserValidatorTest, foreignKeyCanBeFromPendingCommit) {
    otherUser.Id(0);
    Table<User> pendingUnvalidatedTable = {0, true};
    uint8_t pk[User::PublicKeyMaxLength];
    memset(pk, 0x99, sizeof(pk));
    otherUser.PublicKey(pk, sizeof(pk));
    pendingUnvalidatedTable.Save(otherUser, 12);

    Table<User, UserValidator> pendingTable = {0, true};
    User u;
    u.Name().Set("Charlie Day");
    u.Roles(USER_ROLE_ACCOUNT_APPROVER);
    char fb[] = "abc";
    u.FbToken().Set(fb);
    u.CNonce(3);
    u.OtherUserId(otherUser.Id() + 1);
    memset(pk, 0x95, sizeof(pk));
    u.PublicKey().Set(pk, sizeof(pk));
    auto err = pendingTable.Save(u, 12);
    ASSERT_EQ(err, ErrorCode::DbForeignKeyNotValid);
    ASSERT_EQ(err.Details(), "Other user does not exist");
    u.OtherUserId(otherUser.Id());
    ASSERT_EQ(pendingTable.Save(u, 12), ErrorCode::None);
};
