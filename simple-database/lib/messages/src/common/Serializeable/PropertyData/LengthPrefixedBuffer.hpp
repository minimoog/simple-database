#ifndef _LENGTHPREFIXEDBUFFER_HPP_
#define _LENGTHPREFIXEDBUFFER_HPP_

#include "PropertyData.hpp"
#include <array>

template <size_t maxBufferLength>
class LengthPrefixedBuffer : public PropertyData {
    public:
        static const size_t MaxSerializedLength = 4 + maxBufferLength;
        uint32_t length = 0;
        uint8_t buffer[maxBufferLength] = {0};

        ~LengthPrefixedBuffer() {
            LengthPrefixedBuffer::Reset();
        }

        LengthPrefixedBuffer() {};

        LengthPrefixedBuffer(const uint8_t* fromData, uint32_t len) {
            Set(fromData, len);
        }

        size_t TotalLength() const override {
            return length + sizeof(uint32_t);
        }

        void Reset() override {
            length = 0;
            memset(buffer, 0, maxBufferLength);
        }

        uint32_t Serialize(void* toBuffer) const override {
            if (length > maxBufferLength) {
                return sizeof(uint32_t);
            }

            *((uint32_t*)toBuffer) = length;
            memcpy((uint8_t*)toBuffer + sizeof(uint32_t), buffer, length);

            return TotalLength();
        }

        uint32_t Deserialize(const void* fromBuffer) override {
            length = *(uint32_t*) fromBuffer;
            if (length > maxBufferLength) {
                length = 0;
                return 0;
            }
            memcpy(buffer, (uint8_t*)fromBuffer + sizeof(uint32_t), length);
            return TotalLength();
        }

        void Set(const uint8_t* fromData, uint32_t len) {
            if (len > maxBufferLength) {
                length = 0;
                return;
            }
            length = len;
            memcpy(buffer, fromData, length);
        }

        bool IsZero() {
            uint8_t zeros[maxBufferLength] = {0};
            return 0 == memcmp(zeros, buffer, length);
        }

        bool operator==(const LengthPrefixedBuffer<maxBufferLength>& rhs) {
            return length == rhs.length &&
                memcmp(buffer, rhs.buffer, length) == 0;
        }

        bool operator!=(const LengthPrefixedBuffer<maxBufferLength>& rhs) {
            return !(*this == rhs);
        }
};

#endif //_LENGTHPREFIXEDBUFFER_HPP_
