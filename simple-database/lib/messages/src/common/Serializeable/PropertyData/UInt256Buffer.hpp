#ifndef __UINT256BUFFER_HPP_
#define __UINT256BUFFER_HPP_

#include "PropertyData.hpp"
#include <array>

class UInt256Buffer : public PropertyData, public std::array<uint8_t, 32> {
    public:
        static const size_t MaxSerializedLength = 32;
        ~UInt256Buffer();

        UInt256Buffer();
        uint32_t Serialize(void* toBuffer) const override;

        uint32_t Deserialize(const void* fromBuffer) override;

        size_t TotalLength() const override;

        void Reset() override;

        void Set(const uint8_t* fromBuffer);
        void Set(const uint8_t* fromBuffer, size_t length);

        bool IsZero();

        operator uint8_t*() const {
            return (uint8_t*)this->data();
        }
        operator const uint8_t*() const {
            return this->data();
        }
        bool operator==(const UInt256Buffer& rhs);
        bool operator!=(const UInt256Buffer& rhs);
};

#endif //_UINT256BUFFER_HPP_
