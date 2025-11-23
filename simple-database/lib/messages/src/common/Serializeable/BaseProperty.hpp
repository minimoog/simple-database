#ifndef _BASEPROPERTY_HPP_
#define _BASEPROPERTY_HPP_
#include <stdint.h>
#include "PropertyData.hpp"

#define PRIMITIVE_PROPERTY_MASK 0xFF
#define PRIMITIVE_PROPERTY(x) ((x) * 0x01)
#define BUFFER_PROPERTY_MASK 0xFF00
#define BUFFER_PROPERTY(x) ((x) * 0x0100)
#define SET_PROPERTY_MASK 0xFF0000
#define SET_PROPERTY(x) ((x) * 0x010000)

typedef uint32_t property_t;

class BaseProperty {
    public:
        static bool PropertyIsPrimitive(property_t p);
        static bool PropertyIsBuffer(property_t p);
        static bool PropertyIsSet(property_t p);

        enum PropertyType : property_t
        {
            Serializeable = 0,

            UInt8 = PRIMITIVE_PROPERTY(1),
            UInt16 = PRIMITIVE_PROPERTY(2),
            UInt32 = PRIMITIVE_PROPERTY(3),
            UInt64 = PRIMITIVE_PROPERTY(4),
            Bool = PRIMITIVE_PROPERTY(5),
            Enum = PRIMITIVE_PROPERTY(6),

            InternalBuffer = BUFFER_PROPERTY(1),
            ExternalBuffer = BUFFER_PROPERTY(2),
            ConstLengthBuffer = BUFFER_PROPERTY(3),
            String = BUFFER_PROPERTY(4),
            UInt256 = BUFFER_PROPERTY(5),

            LengthEncodedSet = SET_PROPERTY(1),
            StringSet = SET_PROPERTY(2),
            PrimitiveSet = SET_PROPERTY(3),
            SerializeableSet = SET_PROPERTY(4),
            EnumSet = SET_PROPERTY(5),

            Unknown,
        };

        static const uint32_t MaxPropertyNameLength = 255;

        BaseProperty(PropertyType type, const char* name) {
            mType = type;
            strncpy(mName, name, sizeof(mName) - 1);
        }

        virtual uint32_t Serialize(void* toBuffer) const = 0;
        virtual uint32_t Deserialize(const void* fromBuffer) = 0;
        virtual const PropertyData& Data() const =  0;
        PropertyData& Data() {
            return const_cast<PropertyData&>(const_cast<const BaseProperty*>(this)->Data());
        }
        virtual uint32_t MaxLength() const = 0;
        virtual size_t TotalLength() const = 0;
        void Reset()  { Data().Reset(); }
        PropertyType Type() const;
        char* Name() const;

    protected:
        char mName[MaxPropertyNameLength] = {0};
        PropertyType mType;
};

#endif //_BASEPROPERTY_HPP_
