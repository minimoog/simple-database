#ifndef _FIXEDLENGTHSTRING_HPP_
#define _FIXEDLENGTHSTRING_HPP_

#include "PropertyData.hpp"
#include <array>

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

template <size_t maxStringLength>
class FixedLengthString : public PropertyData, public std::array<char, maxStringLength + 1> {
    public:
        static const size_t MaxSerializedLength = 1 + maxStringLength;

        FixedLengthString() {
            FixedLengthString::Reset();
        }

        ~FixedLengthString() {
            FixedLengthString::Reset();
        }

        FixedLengthString(const char * str) {
            FixedLengthString::Reset();
            size_t stringLength = MIN(strlen(str), maxStringLength);
            memcpy(this->data(), str, stringLength);
        }

        size_t TotalLength() const override {
            return strlen(this->data()) + 1;
        }

        uint32_t Serialize(void* toBuffer) const override {
            // using memcpy because gcc yells at me about string truncation
            size_t len = MIN(strlen(this->data()), maxStringLength);
            memcpy(toBuffer, this->data(), len);
            *((char*)toBuffer + len) = '\0';
            return TotalLength();
        }

        uint32_t Deserialize(const void* fromBuffer) override {
            Reset();
            char* p = (char*)fromBuffer;
            while(*(p++)){
                size_t l = p - (char*)fromBuffer;
                if (l >= maxStringLength) {
                    break;
                }
            };
            size_t len = p - (char*)fromBuffer;

            memcpy(this->data(), fromBuffer, len);
            return TotalLength();
        }

        void Reset() override {
            this->fill(0);
        }

        void Set(const char* str)
        {
            memcpy(this->data(), str, MIN(maxStringLength, strlen(str) + 1));
            this->data()[maxStringLength] = '\0';
        }

        operator char*() const {
            return const_cast<char*>(this->data());
        }

        operator const char*() const {
            return this->data();
        }

        bool operator==(const char* rhs) {
            return strncmp(this->data(), rhs, maxStringLength) == 0;
        }

        bool operator==(const char* rhs) const {
            return strncmp(this->data(), rhs, maxStringLength) == 0;
        }

        bool operator!=(const char* rhs) {
            return !(*(this) == rhs);
        }

        bool operator!=(const char* rhs) const {
            return !(*(this) == rhs);
        }

        bool operator==(const FixedLengthString<maxStringLength>& rhs) {
            return static_cast<std::array<char, maxStringLength + 1>>(*this) ==
                static_cast<std::array<char, maxStringLength + 1>>(rhs);
        }

        bool operator!=(const FixedLengthString<maxStringLength>& rhs) {
            return !(*(this) == rhs);
        }
};
#endif //_FIXEDLENGTHSTRING_HPP_
