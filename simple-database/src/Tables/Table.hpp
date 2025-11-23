#ifndef _TABLE_HPP_
#define _TABLE_HPP_
#include <functional>
#include "Serializeable.hpp"
#include "DbDriver.hpp"
#include "ResultSet.hpp"
#include "Query.hpp"
#include "Validator.hpp"
#include "RecordTest.hpp"
#include "NeedleTest.hpp"
#include "CustomTest.hpp"
#include "AllPassTest.hpp"
#include "MaskTest.hpp"

using namespace base_message;
template <class T, class V=void>
class Table {
    public:
        Table(ObjId scope = DbDriver::RootScope, bool pending = false) : mScope(scope), mPending{pending} {}

        virtual bool Find(ObjId id);

        template<typename M> bool FindByMask(const char* propertyName, M mask);
        bool FindBy(const char* propertyName, const void* needle, uint32_t needleLen, bool exactMatch = true);
        bool FindBy(const char* propertyName, const char* needle, bool exactMatch = true);

        template<typename M> ResultSet<T> WhereMask(const char* propertyName, M mask);
        ResultSet<T> Where(const char* propertyName, const void* needle, uint32_t needleLen, bool exactMatch = true);
        ResultSet<T> Where(const char* propertyName, const char* needle, bool exactMatch = true);

        template <typename functor> ResultSet<T> CustomSearch(Query::ResultType resultType, functor customTest);

        ResultSet<T> All();

        template<typename M> ResultSet<T> CountMask(const char* propertyName, M mask);
        ResultSet<T> Count(const char* propertyName, const void* needle, uint32_t needleLen, bool exactMatch = true);
        ResultSet<T> Count(const char* propertyName, const char* needle, bool exactMatch = true);
        ResultSet<T> CountAll();

        Table<T,V>& Not();
        virtual DbError BeforeSave(T&) { return ErrorCode::None; }
        virtual DbError BeforeDelete(T&) { return ErrorCode::None; }
        virtual void AfterSave(T&) {};
        virtual void AfterDelete(ObjId, T&) {};

        bool LoadNextResult(ResultSet<T>& resultSet);
        T& LoadedRecord();
        const ObjId& LoadedRecordCommitId() const { return mRecordCommitId; }
        ResultSet<T> AllWithCommitId(ObjId commitId);
        // Peek ahead at the next id without incrmenting the internel counter
        DbError ValidateCommit(ObjId commitId);

#ifndef DARUMA_DB_RO
        bool DropTable();
        ObjId PeekNextId();
        DbError Save(T& record, ObjId commitId = 0, bool skipValidation = false);
        DbError Save(void* record, ObjId commitId = 0);

        DbError Delete(T& record, bool shouldCallAfterDelete = false);
        DbError Delete(ObjId id, bool shouldCallAfterDelete = false);
        DbError Delete(ResultSet<T>& resultSet, bool shouldCallAfterDelete = false);

        DbError CancelCommit(ObjId commitId);
        DbError CommitAll(ObjId commitId);
        DbError Commit(T& record, ObjId commitId);

#endif //DARUMA_DB_RO

    private:
#ifndef DARUMA_DB_RO
        ObjId NewId(bool increment = true);
#endif
        const char* TableName() const;
        bool LoadNextPage(ResultSet<T>& resultSet);
        bool LoadRecord();
        void Execute(ResultSet<T>& results, RecordTest& test);

        T mRecord;
        ObjId mRecordCommitId = 0;
        bool mNegateNextQuery = false;

    protected:
        const ObjId mScope;
        bool mPending;

};

template <class T, class V>
Table<T,V>& Table<T,V>::Not()
{
    mNegateNextQuery = true;
    return *this;
}

template <class T, class V>
bool Table<T,V>::Find(ObjId id)
{
    DbDriver driver{mScope, mPending};

    if (!(driver.GetRecord(DbDriver::WorkBuffer(), id, TableName()) > 0)) {
        return false;
    }

    return LoadRecord();
}

template <class T, class V>
void Table<T,V>::Execute(ResultSet<T>& results, RecordTest& test)
{
    assert(results.mScope == mScope);
    assert(results.mPending == mPending);

    if (!results.Directory().DidOpen()) {
        results.success = false;
        return;
    }

    uint32_t idPos = mRecord.NonCompactPropertyPosition("Id");
    Query& query = results.GetQuery();

    while (results.Driver().GetNextRecord(DbDriver::WorkBuffer(), results.Directory())) {
        uint64_t id = 0;
        memcpy(&id, (DbDriver::WorkBuffer() + idPos), sizeof(id));
        bool testResult = test(DbDriver::WorkBuffer());

        if ((query.negate && !testResult) || (!query.negate && testResult)) {
            switch(query.resultType) {
                case Query::ResultType::Single: {
                    // We must reload the record
                    // because the work buffer may have been destroyed during a custom search
                    results.success = Find(id);
                    goto loopEnd;
                }
                case Query::ResultType::Many: {
                    results.success = true;
                    results.AppendId(id);
                    if (results.CurrentPageLength() == ResultSet<T>::PageSize) {
                        // if we have filled the page break out early
                        results.HasNextPage(true);
                        goto loopEnd;
                    }
                    break;
                }
                case Query::ResultType::Count: {
                    results.success = Find(id);
                    results.IncCount();
                    break;
                }
            }
        }
    }
    loopEnd:
    return;
}

template <class T, class V>
bool Table<T,V>::FindBy(const char* propertyName, const void* needle, uint32_t needleLen, bool exactMatch)
{
    Query q = FindByNeedleQuery(propertyName, needle, needleLen, exactMatch);
    q.negate = mNegateNextQuery;
    mNegateNextQuery = false;
    ResultSet<T> results{mScope, mPending, q, TableName()};
    NeedleTest<T> test{mRecord, q};
    Execute(results, test);
    return results.success;
}

template <class T, class V>
bool Table<T,V>::FindBy(const char* propertyName, const char* needle, bool exactMatch)
{
    return FindBy(propertyName, needle, strlen(needle) + (uint8_t)exactMatch, exactMatch);
}

template <class T, class V>
template<typename M>
bool Table<T,V>::FindByMask(const char* propertyName, M mask)
{
    Query q = FindByMaskQuery(propertyName, mask);
    q.negate = mNegateNextQuery;
    mNegateNextQuery = false;
    ResultSet<T> result{mScope, mPending, q, TableName()};
    MaskTest<T, M> test{mRecord, q, mask};
    Execute(result, test);
    return result.success;
}

template <class T, class V>
ResultSet<T> Table<T,V>::Count(const char* propertyName, const void* needle, uint32_t needleLen, bool exactMatch)
{
    Query q = CountNeedleQuery(propertyName, needle, needleLen, exactMatch);
    q.negate = mNegateNextQuery;
    mNegateNextQuery = false;
    ResultSet<T> results{mScope, mPending, q, TableName()};
    NeedleTest<T> test{mRecord, q};
    Execute(results, test);
    return results;
}

template <class T, class V>
ResultSet<T> Table<T,V>::Count(const char* propertyName, const char* needle, bool exactMatch)
{
    return Count(propertyName, needle, strlen(needle) + (uint8_t)exactMatch, exactMatch);
}

template <class T, class V>
template<typename M>
ResultSet<T> Table<T,V>::CountMask(const char* propertyName, M mask)
{
    Query q = CountMaskQuery(propertyName, mask);
    q.negate = mNegateNextQuery;
    mNegateNextQuery = false;
    ResultSet<T> results{mScope, mPending, q, TableName()};
    MaskTest<T, M> test{mRecord, q, mask};
    Execute(results, test);
    return results;
}

template <class T, class V>
ResultSet<T> Table<T,V>::CountAll()
{
    Query q = CountAllQuery();

    ResultSet<T> result{mScope, mPending, q, TableName()};
    AllPassTest test;
    Execute(result, test);
    return result;
}

template <class T, class V>
ResultSet<T> Table<T,V>::Where(const char* propertyName, const char* needle, bool exactMatch)
{
    return Where(propertyName, needle, strlen(needle) + (uint8_t)exactMatch, exactMatch);
}

template <class T, class V>
ResultSet<T> Table<T,V>::Where(const char* propertyName, const void* needle, uint32_t needleLen, bool exactMatch)
{
    Query q = WhereNeedleQuery(propertyName, needle, needleLen, exactMatch);
    q.negate = mNegateNextQuery;
    mNegateNextQuery = false;
    ResultSet<T> results{mScope, mPending, q, TableName()};
    NeedleTest<T> test{mRecord, q};
    Execute(results, test);
    return results;
}

template <class T, class V>
template<typename M>
ResultSet<T> Table<T,V>::WhereMask(const char* propertyName, M mask)
{
    Query q = WhereMaskQuery(propertyName, mask);
    q.negate = mNegateNextQuery;
    mNegateNextQuery = false;
    ResultSet<T> results{mScope, mPending, q, TableName()};
    MaskTest<T, M> test{mRecord, q, mask};
    Execute(results, test);
    return results;
}

template <class T, class V>
ResultSet<T> Table<T,V>::All()
{
    Query q = AllQuery();

    ResultSet<T> result{mScope, mPending, q, TableName()};
    AllPassTest test;
    Execute(result, test);
    return result;
}

template <class T, class V>
template<class functor>
ResultSet<T> Table<T,V>::CustomSearch(Query::ResultType resultType, functor customTest)
{
    ResultSet<T> results{mScope, mPending, resultType, TableName(), customTest};

    if (!results.Directory().DidOpen()) {
        results.success = false;
        return results;
    }

    CustomTest<T, functor> test{customTest};
    Execute(results, test);
    return results;
}

template <class T, class V>
bool Table<T,V>::LoadNextPage(ResultSet<T>& resultSet)
{
    if (!resultSet.HasNextPage()) {
        return false;
    }

    resultSet.ClearIds();
    resultSet.HasNextPage(false);

    Query& query = resultSet.GetQuery();
    switch(query.searchType) {
        case Query::SearchType::Needle:
            {
                NeedleTest<T> test{mRecord, query};
                Execute(resultSet, test);
                break;
            }
        case Query::SearchType::Mask:
            {
                switch(query.needleLen) {
                    case sizeof(uint8_t):
                        {
                            MaskTest<T, uint8_t> test{mRecord, query, *(uint8_t*)query.needle};
                            Execute(resultSet, test);
                            break;
                        }
                    case sizeof(uint16_t):
                        {
                            MaskTest<T, uint16_t> test{mRecord, query, *(uint16_t*)query.needle};
                            Execute(resultSet, test);
                            break;
                        }
                    case sizeof(uint32_t):
                        {
                            MaskTest<T, uint32_t> test{mRecord, query, *(uint32_t*)query.needle};
                            Execute(resultSet, test);
                            break;
                        }
                    case sizeof(uint64_t):
                        {
                            MaskTest<T, uint64_t> test{mRecord, query, *(uint64_t*)query.needle};
                            Execute(resultSet, test);
                            break;
                        }
                    default:
                        assert(false);
                        break;
                }
                break;
            }
        case Query::SearchType::All:
            {
                AllPassTest test;
                Execute(resultSet, test);
                break;
            }
        case Query::SearchType::Custom:
            {
                CustomTest<T, std::function<bool(T*)>> test{resultSet.mCustomTest};
                Execute(resultSet, test);
                break;
            }
        case Query::SearchType::RawCustom:
            {
                CustomTest<T, std::function<bool(uint8_t*)>> test{resultSet.mRawCustomTest};
                Execute(resultSet, test);
                break;
            }
    }

    resultSet.ResetIdx();
    return resultSet.success;
}

template <class T, class V>
bool Table<T,V>::LoadNextResult(ResultSet<T>& resultSet)
{
    assert(mScope == resultSet.mScope);
    assert(mPending == resultSet.mPending);
    if (mScope != resultSet.mScope || mPending != resultSet.mPending) {
        return false;
    }

    ObjId id = resultSet.NextId();

    if (id == 0) {
        if (!LoadNextPage(resultSet)) {
            return false;
        }

        id = resultSet.NextId();
    }

    return Find(id);
}

template <class T, class V>
ResultSet<T> Table<T,V>::AllWithCommitId(ObjId commitId)
{
    size_t pos = mRecord.MaxLength();
    // commit ids get saved at the end of the record
    return CustomSearch(
        Query::ResultType::Many,
        [commitId, pos](const uint8_t* recordData) {
            return commitId == *(ObjId*)(recordData + pos);
        }
    );
}

template <class T, class V>
DbError Table<T, V>::ValidateCommit(ObjId commitId)
{
    auto results = AllWithCommitId(commitId);
    if (!results.success) {
        return ErrorCode::None;
    }

    if constexpr (!std::is_void<V>::value) {
        auto validator = V{mScope, mPending};
        while (LoadNextResult(results)) {
            auto error = validator.RecordIsValid(mRecord, commitId);
            if (error) {
                return error;
            }
        }
    }

    return ErrorCode::None;
}

template <class T, class V>
bool Table<T,V>::LoadRecord()
{
    mRecordCommitId = *(ObjId*)(DbDriver::WorkBuffer() + mRecord.MaxLength());
    return mRecord.Deserialize(DbDriver::WorkBuffer(), false) != 0;
}

template <class T, class V>
T& Table<T,V>::LoadedRecord()
{
    return mRecord;
}

template <class T, class V>
const char* Table<T,V>::TableName() const
{
    return T::SerializeableName;
}



#ifndef DARUMA_DB_RO
template <class T, class V>
ObjId Table<T,V>::NewId(bool increment)
{
    ObjId id;
    DbDriver db{mScope, mPending};
    if (!db.NextId(TableName(), id, increment)) {
        return 0;
    }
    return id;
}

template <class T, class V>
ObjId Table<T,V>::PeekNextId()
{
    return NewId(false);
}

template <class T, class V>
bool Table<T,V>::DropTable()
{
    DbDriver db{mScope, mPending};
    return db.DeleteTable(TableName());
}

template <class T, class V>
DbError Table<T,V>::Save(T& record, ObjId commitId, bool skipValidation)
{
    if (mPending && !commitId) {
        return { ErrorCode::General, "Pending data must be saved with a commit id" };
    }
    if (!record.Id()) {
        record.Id(NewId());

        if (!record.Id()) {
            return { ErrorCode::FileWrite, "Unable to provision a new id" };
        }
    }

    if constexpr (!std::is_void<V>::value) {
        if (!skipValidation) {
            auto validator = V{mScope, mPending};
            DbError error = validator.RecordIsValid(record, commitId);
            if (error) {
                return error;
            }
        }
    }

    if (!mPending) {
        DbError preSaveError = BeforeSave(record);
        if (preSaveError != ErrorCode::None) {
            return preSaveError;
        }
    }

    record.Serialize(DbDriver::WorkBuffer(), false);

    DbDriver db{mScope, mPending};
    if (!db.SaveRecord((ObjId)record.Id(), commitId, DbDriver::WorkBuffer(), record.MaxLength(), TableName())) {
        return { ErrorCode::FileWrite, "Unable to write db record to disk" };
    }

    if (!mPending) {
        AfterSave(record);
    }

    return ErrorCode::None;
}

template <class T, class V>
DbError Table<T,V>::Save(void * record, ObjId commitId)
{
    return Save(*(T*)record, commitId);
}

template <class T, class V>
DbError Table<T,V>::Delete(T& record, bool shouldCallAfterDelete)
{
    return Delete((ObjId)record.Id(), shouldCallAfterDelete);
}

template <class T, class V>
DbError Table<T,V>::Delete(ObjId id, bool shouldCallAfterDelete)
{
    if (id == 0) {
        return { ErrorCode::ObjectNotFound, "Id cannot be 0" };
    }

    //load record to be deleted into LoadedRecord()
    if(!Find(id)) {
        return { ErrorCode::ObjectNotFound, "Cannot find record by id" };
    }

    if (!mPending)
    {
        DbError preDeleteError = BeforeDelete(LoadedRecord());
        if (preDeleteError != ErrorCode::None)
        {
            return preDeleteError;
        }
    }

    DbDriver db{mScope, mPending};
    if (!db.DeleteRecord(id, TableName())) {
        return { ErrorCode::FileWrite, "Unable to delete record from disk" };
    }

    if (!mPending || shouldCallAfterDelete) // shouldCallAfterDelete = true In case of CancelCommit().
    {
        AfterDelete(id, LoadedRecord());
    }

    return ErrorCode::None;
}

template <class T, class V>
DbError Table<T,V>::Delete(ResultSet<T>& resultSet, bool shouldCallAfterDelete)
{
    assert(resultSet.mQuery.resultType == Query::ResultType::Many);

    DbError result;
    while (LoadNextResult(resultSet)) {
        auto err = Delete(mRecord, shouldCallAfterDelete);
        if (err) {
            result = err;
        }
    }

    return result;
}

template <class T, class V>
DbError Table<T, V>::CancelCommit(ObjId commitId)
{
    assert(mPending && commitId != 0);

    auto results = AllWithCommitId(commitId);
    if (!results.success) {
        return ErrorCode::None;
    }

    return Delete(results, true);
}

template <class T, class V>
DbError Table<T, V>::CommitAll(ObjId commitId)
{
    assert(mPending && commitId != 0);

    DbError error = ValidateCommit(commitId);
    if (error) {
        return error;
    }

    auto results = AllWithCommitId(commitId);

    if (!results.success) {
        return { ErrorCode::ObjectNotFound, "Unable to find records belonging to commit" };
    }

    while (LoadNextResult(results)) {
        assert(LoadedRecordCommitId() == commitId);
        error = Commit(mRecord, commitId);
        if (error) {
            return error;
        }
    }

    return ErrorCode::None;
}

template <class T, class V>
DbError Table<T, V>::Commit(T& record, ObjId commitId)
{
    assert(mPending && commitId != 0);

    // While saving, set pending to false to target the real table
    mPending = false;

    // Save skipping validation because it should have already been run in `ValidateCommit`
    DbError error = Save(record, commitId, true);

    if (error) {
        return error;
    }

    DbDriver driver{mScope, false};
    driver.SendOnCreateCallbackEvent(T::SerializeableName, record.Id());

    // set pending back to true
    mPending = true;

    // remove the data from the pending table
    error = Delete(record.Id());
    if (error) {
        return error;
    }

    return ErrorCode::None;
}

#endif //DARUMA_DB_RO

#endif //_TABLE_HPP_
