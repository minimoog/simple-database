#ifndef _MASKTEST_HPP_
#define _MASKTEST_HPP_

#include "RecordTest.hpp"
#include "Query.hpp"

template<class T, typename M>
class MaskTest : public RecordTest {
    public:
        MaskTest(T& record, Query& query, M mask) : record(record), query(query), mask(mask) {
            property = record.PropertyByName(query.propertyName);
            assert(property != nullptr);
            assert(BaseProperty::PropertyIsPrimitive(property->Type()));
            propertyPos = record.NonCompactPropertyPosition(query.propertyName);
        }

        bool operator()(void* recordData) override {
            return *(M*)((uint8_t*)recordData + propertyPos) & mask;
        }
    private:
        T& record;
        Query& query;
        M mask;
        BaseProperty* property;
        uint32_t propertyPos;
};

#endif //_MASKTEST_HPP_
