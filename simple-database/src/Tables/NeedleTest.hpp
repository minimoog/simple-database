#ifndef _NEEDLETEST_HPP_
#define _NEEDLETEST_HPP_

#include "RecordTest.hpp"
#include "Query.hpp"

template<class T>
class NeedleTest : public RecordTest {
    public:
        NeedleTest(T& record, Query& query): record(record), query(query) {
            assert(query.searchType == Query::SearchType::Needle);

            property = record.PropertyByName(query.propertyName);

            assert(property != nullptr);

            if (property == nullptr) {
                propertyPos = 0;
                return;
            }

            assert(!BaseProperty::PropertyIsSet(property->Type()));
            assert(property->Type() != BaseProperty::PropertyType::ExternalBuffer);

            propertyPos = record.NonCompactPropertyPosition(query.propertyName);

            if (BaseProperty::PropertyType::InternalBuffer == property->Type() || BaseProperty::PropertyType::ExternalBuffer == property->Type()) {
                comparisonOffset = 4;
            }
        };

        bool operator()(void* recordData) override {
            if (property == nullptr) {
                return false;
            }

            uint32_t len = property->Deserialize((uint8_t*)recordData + propertyPos);
            bool bufferMatch = 0 == memcmp((uint8_t*)recordData + propertyPos + comparisonOffset, query.needle, query.needleLen);
            bool lengthMatch = query.needleLen == len - comparisonOffset;
            return bufferMatch && (!query.exactMatch || lengthMatch);
        }

    private:
        T& record;
        Query& query;
        BaseProperty* property;
        uint32_t propertyPos;
        uint32_t comparisonOffset = 0;
};

#endif //_NEEDLETEST_HPP_
