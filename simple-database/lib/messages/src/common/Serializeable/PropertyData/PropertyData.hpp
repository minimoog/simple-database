#ifndef _PROPERTYDATA_HPP_
#define _PROPERTYDATA_HPP_
#include <stdint.h>
#include <cstring>

class PropertyData {
    public:
        virtual uint32_t Serialize(void* toBuffer) const = 0;
        virtual uint32_t Deserialize(const void* fromBuffer) = 0;
        virtual size_t TotalLength() const = 0;
        virtual void Reset() = 0;
};

#endif //_PROPERTYDATA_HPP_
