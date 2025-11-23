#ifndef _LENGTHPREFIXEDSET_HPP_
#define _LENGTHPREFIXEDSET_HPP_

#include "PropertyData.hpp"
#include <array>

template<class T, size_t maxNumberOfItems>
class LengthPrefixedSet : public PropertyData {
    public:

        static constexpr size_t MaxItemLength() {
            if constexpr(std::is_trivial<T>::value) {
                return sizeof(T);
            } else {
                return T::MaxSerializedLength;
            }
        }

        static const size_t MaxSerializedLength = 4 + maxNumberOfItems *  MaxItemLength();

        ~LengthPrefixedSet() {
            LengthPrefixedSet::Reset();
        }

        LengthPrefixedSet() {
            LengthPrefixedSet::Reset();
        }

        LengthPrefixedSet(const uint8_t* from) {
            LengthPrefixedSet::Reset();
            Deserialize(from);
        }

        uint32_t length = 0;
        std::array<T, maxNumberOfItems> items;

        size_t TotalLength() const override {
            if constexpr (std::is_trivial<T>::value) {
                return sizeof(uint32_t) + length * sizeof(T);
            } else {
                size_t sum = 0;
                for (size_t i = 0; i < length; i++) {
                    sum += items[i].TotalLength();
                }
                return sizeof(uint32_t) + sum;
            }
        }

        bool PushItem(const T& item) {
            if (length >= maxNumberOfItems) {
                return false;
            }
            items[length++] = item;
            return true;
        }

        void Reset() override {
            length = 0;
            if constexpr (std::is_trivial<T>::value) {
                memset(items.data(), 0, sizeof(items));
            } else {
                for (auto& item : items) {
                    item.Reset();
                }
            }
        }

        uint32_t Serialize(void* toBuffer) const override {
            if (length > maxNumberOfItems) {
                *(uint32_t*)toBuffer = 0;
                return sizeof(uint32_t);
            }

            *(uint32_t*)toBuffer = length;
            uint32_t pos = sizeof(uint32_t);
            for (size_t i = 0; i < length; i++) {
                if constexpr (std::is_trivial<T>::value) {
                    memcpy((uint8_t*)toBuffer + pos, &items[i], sizeof(T));
                    pos += sizeof(T);
                } else {
                    pos += items[i].Serialize((uint8_t*)toBuffer + pos);
                }
            }
            return pos;
        }

        uint32_t Deserialize(const void* fromBuffer) override {
            length = *(uint32_t*)fromBuffer;
            if (length > maxNumberOfItems) {
                length = 0;
                return 0;
            }

            uint32_t pos = sizeof(uint32_t);
            for (size_t i = 0; i < length; i++) {
                if constexpr (std::is_trivial<T>::value) {
                    memcpy(&items[i], (uint8_t*)fromBuffer + pos, sizeof(T));
                    pos += sizeof(T);
                } else {
                    pos += items[i].Deserialize((uint8_t*)fromBuffer + pos);
                }
            }
            return pos;
        }

        T& operator[](size_t i) {
            return items[i];
        }

        const T& operator[](size_t i) const {
            return items[i];
        }
};

#endif //_LENGTHPREFIXEDSET_HPP_
