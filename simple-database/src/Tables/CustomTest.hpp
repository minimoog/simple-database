#ifndef _CUSTOMTEST_HPP_
#define _CUSTOMTEST_HPP_

#include "RecordTest.hpp"
#include "Query.hpp"
#include <functional>

template<class T, typename Functor>
class CustomTest : public RecordTest {
    public:
        CustomTest(Functor test) : test{test} {}

        bool operator()(void* recordData) override {
            if constexpr (std::is_convertible<Functor, std::function<bool(T*)>>::value) {
                T record;
                record.Deserialize((uint8_t*)recordData, false);
                return test((T*)&record);
            } else {
                return test((uint8_t*)recordData);
            }
        }
    private:
        Functor test;
};

#endif //_CUSTOMTEST_HPP_
