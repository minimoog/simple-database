#ifndef _ENUMSET_HPP_
#define _ENUMSET_HPP_

#include "PropertyData.hpp"
#include "LengthPrefixedSet.hpp"
#include "MessageDefinitions.hpp"

template <typename T, size_t maxNumberOfItems>
class EnumSet : public LengthPrefixedSet<T, maxNumberOfItems> {
    public:
        static const size_t MaxSerializedLength = 4 + maxNumberOfItems * (base_message::EnumMaxLength + 1);
};
#endif //_ENUMSET_HPP_
