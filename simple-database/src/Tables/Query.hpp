#ifndef _QUERY_HPP_
#define _QUERY_HPP_
#include <stdint.h>
#include <assert.h>
#include "BaseProperty.hpp"

#define FindByNeedleQuery(property, needle, needleLen, exactMatch) \
    Query(Query::ResultType::Single, Query::SearchType::Needle, property, needle, needleLen, exactMatch)

#define FindByMaskQuery(property, mask) \
    Query(Query::ResultType::Single, Query::SearchType::Mask, property, &(mask), sizeof(mask), true)

#define WhereNeedleQuery(property, needle, needleLen, exactMatch) \
    Query(Query::ResultType::Many, Query::SearchType::Needle, property, needle, needleLen, exactMatch)

#define WhereMaskQuery(property, mask) \
    Query(Query::ResultType::Many, Query::SearchType::Mask, property, &(mask), sizeof(mask), true)

#define AllQuery() \
    Query(Query::ResultType::Many, Query::SearchType::All, nullptr, nullptr, 0, false)

#define FirstQuery() \
    Query(Query::ResultType::Single, Query::SearchType::All, nullptr, nullptr, 0, false)

#define CountNeedleQuery(property, needle, needleLen, exactMatch) \
    Query(Query::ResultType::Count, Query::SearchType::Needle, property, needle, needleLen, exactMatch)

#define CountMaskQuery(property, mask) \
    Query(Query::ResultType::Count, Query::SearchType::Mask, property, &(mask), sizeof(mask), true)

#define CountAllQuery() \
    Query(Query::ResultType::Count, Query::SearchType::All, nullptr, nullptr, 0, false)

#define CustomQuery(resultType) \
    Query(resultType, Query::SearchType::Custom, nullptr, nullptr, 0, false)

#define RawCustomQuery(resultType) \
    Query(resultType, Query::SearchType::RawCustom, nullptr, nullptr, 0, false)

class Query {
    public:
        enum class ResultType {
            Single,
            Many,
            Count,
        };
        enum class SearchType {
            Needle,
            Mask,
            All,
            Custom,
            RawCustom,
        };
        static const uint32_t MaxNeedleLength = 255;

        Query() {}

        Query(ResultType resultType, SearchType searchType, const char* propertyName, const void* needle, uint32_t needleLen, bool exactMatch);
        uint8_t needle[MaxNeedleLength];
        uint32_t needleLen = 0;
        bool exactMatch = false;
        bool negate = false;
        char propertyName[BaseProperty::MaxPropertyNameLength];
        ResultType resultType;
        SearchType searchType;
};
#endif //_QUERY_HPP_
