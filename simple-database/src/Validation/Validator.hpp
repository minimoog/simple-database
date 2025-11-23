#ifndef _VALIDATOR_HPP_
#define _VALIDATOR_HPP_
#include <string.h>
#include "DbDriver.hpp"
#include "ResultSet.hpp"
#include "MessageDefinitions.hpp"
#include "MessageEnums.hpp"
#include "DbError.hpp"

#define NOT_EMPTY_VALIDATION(propertyName, details) \
    if (!NotEmptyValidation(record.PropertyByName(propertyName))) \
        return { base_message::ErrorCode::DbNotEmptyValidation, details };

#define NOT_ZERO_VALIDATION(propertyName, details) \
    if (!NotZeroValidation(record.PropertyByName(propertyName))) \
        return { base_message::ErrorCode::DbNotZeroValidation, details };

#define ENUM_NOT_ZERO_VALIDATION(propertyName, enum, details) \
    if (!EnumNotZeroValidation<enum##Wrapper>(record.PropertyByName(propertyName))) \
        return { base_message::ErrorCode::DbNotZeroValidation, details };

#define FORIEGN_KEY_VALIDATION(klass, id, details) \
    if (!ForeignKeyValidation<Table<klass>>(id, commitId)) \
        return { base_message::ErrorCode::DbForeignKeyNotValid, details };

// unique validations should check uncommitted & committed data
#define UNIQUE_VALIDATION(klass, propertyName, details) \
    if (!UniqueValidation<Table<klass>>(record.PropertyByName(propertyName), record.Id(), false)) \
        return { base_message::ErrorCode::DbRecordNotUnique, details }; \
    if (mPending && !UniqueValidation<Table<klass>>(record.PropertyByName(propertyName), record.Id(), true)) \
        return { base_message::ErrorCode::DbRecordNotUnique, details };

template <class T>
class Validator {
    public:
        explicit Validator(ObjId scope, bool pending) : mScope(scope), mPending{pending} {}
        virtual DbError RecordIsValid(T& record, ObjId commitId) = 0;
    protected:
        bool NotEmptyValidation(const BaseProperty* property);
        bool NotZeroValidation(const BaseProperty* property);
        template <typename EnumWrapper> bool EnumNotZeroValidation(const BaseProperty* property);
        template <class Table> bool ForeignKeyValidation(ObjId id, ObjId commitId);
        template <class Table> bool UniqueValidation(const BaseProperty* property, ObjId id, bool preCommit);
        ObjId mScope;
        bool mPending;
};

template <class T>
bool Validator<T>::NotEmptyValidation(const BaseProperty* property)
{
    size_t minLength = 4;
    if (property->Type() == BaseProperty::PropertyType::String) {
        minLength = 1;
    }
    return property->Data().TotalLength() != minLength;
}

template <class T>
bool Validator<T>::NotZeroValidation(const BaseProperty* property)
{
    uint8_t *temp = DbDriver::WorkBuffer();
    uint32_t l = property->Serialize(temp);
    if (l == 0)
        return false;
    // Compare the buffer is not zero by looking at the buffer and the buffer with offset of 1
    // if the entire buffer is zero it will return true if there is a difference even in one location it will return false
    // the actual return is negated because we check if the buffer is not zero
    return !((*temp == 0) && (memcmp(temp, temp + 1, l-1) == 0));
}

template <class T>
template <typename EnumWrapper>
bool Validator<T>::EnumNotZeroValidation(const BaseProperty* property)
{
    char value[base_message::EnumMaxLength] = {0};
    property->Serialize(value);

    char zeroStr[base_message::EnumMaxLength] = {0};
    auto e = EnumWrapper(0);
    e.Serialize(zeroStr);

    return strncmp(zeroStr, value, base_message::EnumMaxLength) != 0;
}

template <class T>
template <class Table>
bool Validator<T>::ForeignKeyValidation(ObjId id, ObjId commitId) {
    Table mainForeignTable{mScope};
    if (!id) {
        return true;
    }

    if (mainForeignTable.Find(id)) {
        return true;
    }

    if (!mPending) {
        return false;
    }

    // Foreign keys are allowed to refence records that are part of the same commit
    Table pendingForeignTable{mScope, true};
    auto results = pendingForeignTable.AllWithCommitId(commitId);

    while (pendingForeignTable.LoadNextResult(results)) {
        if (pendingForeignTable.LoadedRecord().Id() == id) {
            return true;
        }
    }

    return false;

}

template <class T>
template <class Table>
bool Validator<T>::UniqueValidation(const BaseProperty* property, ObjId id, bool preCommit)
{
    Table table{mScope, preCommit};

    uint8_t *serialized = DbDriver::WorkBuffer();
    uint32_t len = property->Serialize(serialized);

    uint32_t offset = 0;
    if (BaseProperty::PropertyType::InternalBuffer == property->Type() || BaseProperty::PropertyType::ExternalBuffer == property->Type()) {
        offset = sizeof(uint32_t);

        // Don't reject empty buffers
        if (len == offset) {
            return true;
        }
    }

    // Don't reject empty strings
    if (BaseProperty::PropertyType::String == property->Type() && len == 1) {
        return true;
    }

    ResultSet<T> results = table.Where(property->Name(), (uint8_t*)serialized + offset, len - offset);

    // if there are no results then its cool
    if (results.CurrentPageLength() == 0) {
        return true;
    }

    // if there are more than one result its def not cool
    if (results.CurrentPageLength() > 1) {
        return false;
    }

    // if there is one result and its our record, that's fine otherwise not ok
    return results.NextId() == id;
}

#endif //_VALIDATOR_HPP_
