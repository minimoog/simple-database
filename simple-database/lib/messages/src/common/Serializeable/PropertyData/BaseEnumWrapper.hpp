#ifndef _BASEENUMWRAPPER_HPP_
#define _BASEENUMWRAPPER_HPP_

#include "PropertyData.hpp"
#include "MessageDefinitions.hpp"

using EnumString = FixedLengthString<base_message::EnumMaxLength>;

template <typename Enum>
class BaseEnumWrapper : public PropertyData {
    public:
        static const size_t MaxSerializedLength = base_message::EnumMaxLength + 1;

        BaseEnumWrapper() {
            BaseEnumWrapper::Reset();
        }

        virtual ~BaseEnumWrapper() {
            BaseEnumWrapper::Reset();
        }

        uint32_t Serialize(void* toBuffer) const override {
            return ToString().Serialize(toBuffer);
        }

        uint32_t Deserialize(const void* fromBuffer) override {
            auto str = EnumString();
            uint32_t len = str.Deserialize(fromBuffer);
            FromString(str);
            return len;
        }

        size_t TotalLength() const override {
            return ToString().TotalLength();
        }

        void Reset() override {
            memset(&value, 0, sizeof(Enum));
        }

        void Set(Enum val) {
            value = val;
        };

        friend bool operator==(const BaseEnumWrapper<Enum>& lhs, const Enum& rhs) {
            return lhs.value == rhs;
        }

        bool operator!=(Enum rhs) {
            return !(*(this) == rhs);
        }

        operator Enum() {
            return value;
        }

        operator Enum() const {
            return value;
        }

        virtual EnumString ToString() const = 0;
        virtual void FromString(const EnumString&) = 0;

        Enum value;
};

#endif //_BASEENUMWRAPPER_HPP_
