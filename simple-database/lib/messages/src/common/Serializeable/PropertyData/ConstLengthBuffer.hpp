#ifndef _CONSTLENGTHBUFFER_HPP_
#define _CONSTLENGTHBUFFER_HPP_

#include "PropertyData.hpp"
#include <array>

template <size_t length>
class ConstLengthBuffer : public PropertyData, public std::array<uint8_t, length> {
    public:
        static const size_t MaxSerializedLength = length;
        ~ConstLengthBuffer() {
            ConstLengthBuffer::Reset();
        }

        ConstLengthBuffer() {
            ConstLengthBuffer::Reset();
        }

        uint32_t Serialize(void* toBuffer) const override {
            memcpy(toBuffer, this->data(), length);
            return length;
        }

        uint32_t Deserialize(const void* fromBuffer) override {
            memcpy(this->data(), fromBuffer, length);
            return length;
        }

        size_t TotalLength() const override { return length; }

        void Reset() override {
            this->fill(0);
        }

        void Set(const uint8_t* fromBuffer) {
            Deserialize(fromBuffer);
        }

        bool IsZero() {
            uint8_t zeros[length] = {0};
            return 0 == memcmp(zeros, this->data(), length);
        }

        operator uint8_t*() const {
            return (uint8_t*)this->data();
        }
        operator const uint8_t*() const {
            return this->data();
        }
        bool operator==(const ConstLengthBuffer<length>& rhs) {
            return static_cast<std::array<uint8_t, length>>(*this) == static_cast<std::array<uint8_t, length>>(rhs);
        }
        bool operator!=(const ConstLengthBuffer<length>& rhs) {
            return !(*(this) == rhs);
        }
};
#endif //_CONSTLENGTHBUFFER_HPP_
