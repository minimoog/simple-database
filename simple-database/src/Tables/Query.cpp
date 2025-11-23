#include "Query.hpp"
Query::Query(ResultType resultType, SearchType searchType, const char* _propertyName, const void* _needle, uint32_t _needleLen, bool _exactMatch)
    : resultType(resultType), searchType(searchType)
{
    assert(_needleLen <= MaxNeedleLength);
    if (_needleLen > MaxNeedleLength) {
        return;
    }

    if (_propertyName != nullptr) {
        strncpy(propertyName, _propertyName, sizeof(propertyName) - 1);
    }

    memcpy(needle, _needle, _needleLen);
    needleLen = _needleLen;
    exactMatch = _exactMatch;
 }
