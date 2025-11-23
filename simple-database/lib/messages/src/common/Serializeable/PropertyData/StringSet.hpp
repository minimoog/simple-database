#ifndef _STRINGSET_HPP_
#define _STRINGSET_HPP_

#include "PropertyData.hpp"
#include "FixedLengthString.hpp"
#include "LengthPrefixedSet.hpp"

template <size_t maxNumberOfItems, size_t maxItemLength>
class StringSet : public LengthPrefixedSet<FixedLengthString<maxItemLength>, maxNumberOfItems> {
    public:
        static const size_t MaxSerializedLength = 4 + maxNumberOfItems * (maxItemLength + 1);
};

#endif //_STRINGSET_HPP_
