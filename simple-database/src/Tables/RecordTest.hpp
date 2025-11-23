#ifndef _RECORDTEST_HPP_
#define _RECORDTEST_HPP_
#include "BaseMessageDefinitions.hpp"

class RecordTest {
    public:
        virtual bool operator()(void* recordData) = 0;
};
#endif //_RECORDTEST_HPP_
