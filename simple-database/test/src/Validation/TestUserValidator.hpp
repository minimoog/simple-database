#ifndef _TESTUSERVALIDATION_HPP_
#define _TESTUSERVALIDATION_HPP_

#include "Validator.hpp"
#include "TestUser.hpp"

class TestUserValidator : public Validator<TestUser> {
    public:
        explicit TestUserValidator(ObjId scope, ObjId preCommitId) : Validator<TestUser>(scope, preCommitId) {}
        DbError RecordIsValid(TestUser& record, ObjId commitId) override;
};
#endif //_TESTUSERVALIDATION_HPP_
