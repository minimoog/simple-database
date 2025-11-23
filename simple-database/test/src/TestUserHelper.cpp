#include "TestUserHelper.hpp"

using namespace test_user;
bool test_user::isAccountApprover(TestUser& u)
{
    return (u.Roles() & USER_ROLE_ACCOUNT_APPROVER) != 0;
}

bool test_user::isAccountManager(TestUser& u)
{
    return (u.Roles() & USER_ROLE_ACCOUNT_MANAGER) != 0;
}

bool test_user::isAccountInitiator(TestUser& u)
{
    return (u.Roles() & USER_ROLE_ACCOUNT_INITIATOR) != 0;
}

bool test_user::isOrganizationApprover(TestUser& u)
{
    return (u.Roles() & USER_ROLE_ORGANIZATION_APPROVER) != 0;
}

bool test_user::isOrganizationManager(TestUser& u)
{
    return (u.Roles() & USER_ROLE_ORGANIZATION_MANAGER) != 0;
}

bool test_user::isPermissionsManager(TestUser& u)
{
    return (u.Roles() & USER_ROLE_PERMISSIONS_MANAGER) != 0;
}

bool test_user::isCryptoOfficer(TestUser& u)
{
    return (u.Roles() & USER_ROLE_CRYPTO_OFFICER) != 0;
}

bool test_user::isMantainance(TestUser& u)
{
    return (u.Roles() & USER_ROLE_MAINTENANCE) != 0;
}
