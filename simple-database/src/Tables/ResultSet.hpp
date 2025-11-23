#ifndef _RESULTSET_HPP_
#define _RESULTSET_HPP_
#include <stdint.h>
#include <functional>
#include "Query.hpp"
#include "DbDriver.hpp"

template <class T>
class ResultSet {
    public:
        ResultSet(ObjId scope, bool pending, const Query& query, const char* tableName) : mScope{scope}, mPending{pending}, mQuery(query), mDbDriver(scope, pending) {
            mDbDriver.OpenTable(tableName, mDirectory);
        }

        template<typename M>
        ResultSet(ObjId scope, bool pending, Query::ResultType resultType, const char* tableName, M customTest) : mScope{scope}, mPending{pending}, mDbDriver(scope, pending)
        {
            if constexpr (std::is_convertible<M, std::function<bool(T*)>>::value) {
                mQuery = CustomQuery(resultType);
                mCustomTest = customTest;
            } else {
                mQuery = RawCustomQuery(resultType);
                mRawCustomTest = customTest;
            }

            mDbDriver.OpenTable(tableName, mDirectory);
        }

        static const uint8_t PageSize = 0xFF;
        uint64_t NextId();
        bool HasNextPage();
        uint32_t GetCount();
        bool success = false;
    private:
        void HasNextPage(bool hasNextPage);
        void AppendId(uint64_t id);
        Query& GetQuery();
        void IncCount();
        uint8_t CurrentPageLength();
        DbDriver& Driver();
        DirectoryWrapper& Directory();
        void ResetIdx();
        void ClearIds();

        ObjId mScope;
        bool mPending;
        uint64_t mIds[PageSize] = {0};
        uint8_t mCurrentPageLength = 0;
        bool mHasNextPage = false;
        uint32_t mCount = 0;
        Query mQuery;
        DbDriver mDbDriver;
        DirectoryWrapper mDirectory;
        std::function<bool(T*)> mCustomTest = nullptr;
        std::function<bool(uint8_t*)> mRawCustomTest = nullptr;

        uint8_t mResultIdx = 0;

        template<typename, typename> friend class Table;
        template<typename> friend class Validator;
};

template <class T>
uint8_t ResultSet<T>::CurrentPageLength()
{
    return mCurrentPageLength;
}

template <class T>
uint64_t ResultSet<T>::NextId()
{
    if (mResultIdx < mCurrentPageLength) {
        return mIds[mResultIdx++];
    }

    return 0;
}

template <class T>
void ResultSet<T>::ClearIds()
{
    mCurrentPageLength = 0;
    memset(mIds, 0, sizeof(mIds));
}

template <class T>
void ResultSet<T>::ResetIdx()
{
    mResultIdx = 0;
}

template <class T>
void ResultSet<T>::AppendId(uint64_t id)
{
    assert(mCurrentPageLength <= PageSize);

    mIds[mCurrentPageLength] = id;
    mCurrentPageLength++;
}

template <class T>
Query& ResultSet<T>::GetQuery()
{
    return mQuery;
}

template <class T>
bool ResultSet<T>::HasNextPage()
{
    return mHasNextPage;
}

template <class T>
void ResultSet<T>::HasNextPage(bool hasNextPage)
{
    mHasNextPage = hasNextPage;
}

template <class T>
uint32_t ResultSet<T>::GetCount()
{
    assert(mQuery.resultType == Query::ResultType::Count);
    return mCount;

}

template <class T>
void ResultSet<T>::IncCount()
{
    assert(mQuery.resultType == Query::ResultType::Count);
    mCount++;

}

template <class T>
DbDriver& ResultSet<T>::Driver()
{
    return mDbDriver;
}

template <class T>
DirectoryWrapper& ResultSet<T>::Directory()
{
    return mDirectory;
}
#endif //_RESULTSET_HPP_
