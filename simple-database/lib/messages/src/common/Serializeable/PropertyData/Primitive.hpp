#ifndef _PRIMITIVE_HPP_
#define _PRIMITIVE_HPP_

#include "PropertyData.hpp"
#include <type_traits>

template <typename T>
class Primitive : public PropertyData {
    static_assert(std::is_trivial<T>::value, "Primitives must be trivial");

    public:
        static const size_t MaxSerializedLength = sizeof(T);

        Primitive() {
            Primitive::Reset();
        }

        ~Primitive() {
            Primitive::Reset();
        }

        uint32_t Serialize(void* toBuffer) const override {
            memcpy(toBuffer, &value, sizeof(T));
            return sizeof(T);
        }
        uint32_t Deserialize(const void* fromBuffer) override {
            memcpy(&value, fromBuffer, sizeof(T));
            return sizeof(T);
        }

        size_t TotalLength() const override {
            return sizeof(T);
        }

        void Reset() override {
            memset(&value, 0, sizeof(T));
        }

        T value;
};

#endif //_PRIMITIVE_HPP_
