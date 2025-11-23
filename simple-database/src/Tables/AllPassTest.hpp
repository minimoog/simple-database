#ifndef _ALLPASSTEST_HPP_
#define _ALLPASSTEST_HPP_
#include "AllPassTest.hpp"

class AllPassTest : public RecordTest {
    public:
        virtual bool operator()(void* recordData) {
            (void)recordData;
            return true;
        };
};
#endif //_ALLPASSTEST_HPP_
