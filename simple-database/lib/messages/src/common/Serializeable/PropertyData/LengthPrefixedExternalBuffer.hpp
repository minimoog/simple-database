#ifndef _LENGTHPREFIXEDEXTERNALBUFFER_HPP_
#define _LENGTHPREFIXEDEXTERNALBUFFER_HPP_

#include "PropertyData.hpp"
#include <cassert>

template <size_t maxBufferLength>
class LengthPrefixedExternalBuffer : public PropertyData {
    public:
        static const size_t MaxSerializedLength = 4 + maxBufferLength;
        LengthPrefixedExternalBuffer(uint8_t* buffer) : buffer{buffer} { }
        uint8_t* buffer = nullptr;
        uint32_t length = 0;

        LengthPrefixedExternalBuffer() = default;

        ~LengthPrefixedExternalBuffer() {
            LengthPrefixedExternalBuffer::Reset();
        }

        LengthPrefixedExternalBuffer(uint8_t* data, uint32_t len) {
            if (length <= maxBufferLength) {
                length = len;
            }
            buffer = data;
        }

        size_t TotalLength() const override {
            return length + sizeof(uint32_t);
        }

        void Reset() override {
            length = 0;
            buffer = nullptr;
        }

        uint32_t Serialize(void* toBuffer) const override {
            *(uint32_t*)toBuffer = length;
            memcpy((uint8_t*)toBuffer + sizeof(uint32_t), buffer, length);
            return TotalLength();
        }

        uint32_t Deserialize(const void* fromBuffer) override {
            length = *(uint32_t*)fromBuffer;
            if (length > maxBufferLength) {
                length = 0;
                return 0;
            }

            assert(buffer);
            if (!buffer) {
                return TotalLength();
            }

            memcpy(buffer, (uint8_t*)fromBuffer + sizeof(uint32_t), length);
            return TotalLength();
        }

        void Set(const uint8_t* data, uint32_t len) {
            if (len > maxBufferLength) {
                length = 0;
                return;
            }
            length = len;
            memcpy(buffer, data, length);
        }

        void SetBuffer(uint8_t* buff)  {
            buffer = buff;
        }

        bool IsZero() {
            uint8_t zeros[maxBufferLength] = {0};
            return 0 == memcmp(zeros, buffer, length);
        }

        bool operator==(const LengthPrefixedExternalBuffer<maxBufferLength>& rhs) {
            return length == rhs.length &&
                memcmp(buffer, rhs.buffer, length) == 0;
        }

        bool operator!=(const LengthPrefixedExternalBuffer<maxBufferLength>& rhs) {
            return !(*this == rhs);
        }
};

#endif //_LENGTHPREFIXEDEXTERNALBUFFER_HPP_
