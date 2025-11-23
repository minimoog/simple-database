#ifndef _LENGTHENCODEDSET_HPP_
#define _LENGTHENCODEDSET_HPP_

#include "PropertyData.hpp"
#include "LengthPrefixedBuffer.hpp"
#include "LengthPrefixedSet.hpp"

template <size_t maxNumberOfItems, size_t maxItemLength>
class LengthEncodedSet : public LengthPrefixedSet<LengthPrefixedBuffer<maxItemLength>, maxNumberOfItems> {
    public:
        static const size_t MaxSerializedLength = 4 + maxNumberOfItems * (maxItemLength + 4);
        LengthEncodedSet() = default;

        ~LengthEncodedSet() {
            this->Reset();
        }
};
#endif //_LENGTHENCODEDSET_HPP_
