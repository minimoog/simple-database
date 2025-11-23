#ifndef _USERHELPER_HPP_
#define _USERHELPER_HPP_

#include <stdint.h>
#include <stddef.h>
#include "TestUser.hpp"

#define USER_ROLE_ACCOUNT_APPROVER          1
#define USER_ROLE_ACCOUNT_MANAGER           2
#define USER_ROLE_ACCOUNT_INITIATOR         4
#define USER_ROLE_ACCOUNT_AUDITOR           8

#define USER_ROLE_ORGANIZATION_APPROVER     16
#define USER_ROLE_ORGANIZATION_MANAGER      32

#define USER_ROLE_PERMISSIONS_MANAGER       64

#define USER_ROLE_CRYPTO_OFFICER            128
#define USER_ROLE_MAINTENANCE               256
namespace test_user {
    typedef uint16_t Roles;
    static const Roles MiniMVP_admin = USER_ROLE_ORGANIZATION_MANAGER;
    static const Roles MiniMVP_user = USER_ROLE_ACCOUNT_APPROVER;

    bool isAccountApprover(TestUser& u);
    bool isAccountManager(TestUser& u);
    bool isAccountInitiator(TestUser& u);
    bool isAccountAuditor(TestUser& u);
    bool isOrganizationApprover(TestUser& u);
    bool isOrganizationManager(TestUser& u);
    bool isPermissionsManager(TestUser& u);
    bool isCryptoOfficer(TestUser& u);
    bool isMantainance(TestUser& u);
}
#endif //_USERHELPER_HPP_
