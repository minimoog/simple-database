#ifndef _PROPERTY_HPP_
#define _PROPERTY_HPP_

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <type_traits>
#include "BaseProperty.hpp"
#include "PropertyData.hpp"
#include "Primitive.hpp"
#include "FixedLengthString.hpp"
#include "LengthEncodedSet.hpp"
#include "StringSet.hpp"
#include "LengthPrefixedBuffer.hpp"
#include "LengthPrefixedExternalBuffer.hpp"
#include "ConstLengthBuffer.hpp"
#include "BaseEnumWrapper.hpp"
#include "EnumSet.hpp"
#include "UInt256Buffer.hpp"
#define PrimitiveProperty(name, T) Property<Primitive<T>>(PrimitiveToPropertyType<T>(), name)

#define InternalBufferProperty(name, bufferLength) Property<LengthPrefixedBuffer<bufferLength>>(BaseProperty::PropertyType::InternalBuffer, name)
#define ExternalBufferProperty(name, bufferLength) Property<LengthPrefixedExternalBuffer<bufferLength>>(BaseProperty::PropertyType::ExternalBuffer, name)
#define ConstLengthBufferProperty(name, bufferLength) Property<ConstLengthBuffer<bufferLength>>(BaseProperty::PropertyType::ConstLengthBuffer, name)
#define StringProperty(name, stringLength) Property<FixedLengthString<stringLength>>(BaseProperty::PropertyType::String, name)
#define EnumProperty(name, EnumClass) Property<EnumClass>(BaseProperty::PropertyType::Enum, name)

#define StringSetProperty(name, itemLength, numberOfItems) Property<StringSet<numberOfItems, itemLength>>(BaseProperty::PropertyType::StringSet, name)
#define LengthEncodedSetProperty(name, itemLength, numberOfItems) Property<LengthEncodedSet<numberOfItems, itemLength>>(BaseProperty::PropertyType::LengthEncodedSet, name)
#define PrimitiveSetProperty(name, T, numberOfItems) Property<LengthPrefixedSet<T, numberOfItems>>(BaseProperty::PropertyType::PrimitiveSet, name)
#define SerializeableSetProperty(name, T, numberOfItems) Property<LengthPrefixedSet<T, numberOfItems>>(BaseProperty::PropertyType::SerializeableSet, name)
#define EnumSetProperty(name, EnumClass, numberOfItems) Property<EnumSet<EnumClass, numberOfItems>>(BaseProperty::PropertyType::EnumSet, name)

#define SerializeableProperty(name, T) Property<T>(BaseProperty::PropertyType::Serializeable, name)
#define UInt256Property(name) Property<UInt256Buffer>(BaseProperty::PropertyType::UInt256, name)

template<typename T>
static constexpr BaseProperty::PropertyType PrimitiveToPropertyType() {
    static_assert(
        std::is_trivial_v<T>,
        "type passed to `PrimitiveToPropertyType` must be trivial"
    );

    if constexpr (std::is_same_v<T, uint8_t>) {
        return BaseProperty::PropertyType::UInt8;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        return BaseProperty::PropertyType::UInt16;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return BaseProperty::PropertyType::UInt32;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        return BaseProperty::PropertyType::UInt64;
    } else if constexpr (std::is_same_v<T, bool>) {
        return BaseProperty::PropertyType::Bool;
    } else {
        assert(false);
        return BaseProperty::PropertyType::Unknown;
    }
};

template <class DataClass>
class Property : public BaseProperty
{
    static_assert(
        std::is_base_of_v<PropertyData, DataClass>,
        "`Property<DataCass>` may only be used if `DataClass` inherits from `PropertyData`"
    );

    public:
        Property(PropertyType type, const char* name);

        uint32_t MaxLength() const override;
        size_t TotalLength() const override { return data.TotalLength(); };

        uint32_t Serialize(void* toBuffer) const override;
        uint32_t Deserialize(const void* fromBuffer) override;

        const PropertyData& Data() const override;
        DataClass data;
};

template <class DataClass>
const PropertyData& Property<DataClass>::Data() const
{
    return data;
}

template <class DataClass>
Property<DataClass>::Property(PropertyType type, const char* name) : BaseProperty(type, name) { }

template <class DataClass>
uint32_t Property<DataClass>::MaxLength() const
{
    return DataClass::MaxSerializedLength;
}

template <class DataClass>
uint32_t Property<DataClass>::Serialize(void* toBuffer) const
{
    return data.Serialize(toBuffer);
}

template <class DataClass>
uint32_t Property<DataClass>::Deserialize(const void* fromBuffer)
{
    return data.Deserialize(fromBuffer);
}

#endif //_PROPERTY_HPP_
