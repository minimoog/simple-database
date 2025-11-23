#include "TestUserValidator.hpp"
#include "Table.hpp"

DbError TestUserValidator::RecordIsValid(TestUser& record, ObjId commitId)
{
    NOT_EMPTY_VALIDATION("Name", "Name can't be empty");

    NOT_EMPTY_VALIDATION("PublicKey", "Public key can't be empty");

    UNIQUE_VALIDATION(TestUser, "PublicKey", "Public key must be unique");

    NOT_ZERO_VALIDATION("Roles", "User must have at least one role");

    FORIEGN_KEY_VALIDATION(TestUser, record.OtherUserId(), "Other user does not exist");

    return base_message::ErrorCode::None;
}
