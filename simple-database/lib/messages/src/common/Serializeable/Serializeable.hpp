
#ifndef __SERIALIZEABLE_H_
#define __SERIALIZEABLE_H_
#include "Property.hpp"
#include "PropertyData.hpp"

class BaseMessage;

class Serializeable : public PropertyData
{
    public:
        Serializeable() = default;
        virtual ~Serializeable() { Serializeable::Reset(); }

        uint32_t Serialize(void* toBuffer) const override { return Serialize(toBuffer, true); }
        uint32_t Deserialize(const void* fromBuffer) override { return Deserialize(fromBuffer, true); }
        size_t TotalLength() const override;

        uint32_t Serialize(void* buffer, bool compact) const;
        uint32_t Deserialize(const void* buffer, bool compact);
        void Reset() override;

        virtual uint32_t MaxLength() const;
        virtual uint32_t NumberOfProperties() const;

        virtual const BaseProperty* const* Properties() const;
        BaseProperty* const* Properties() {
            return const_cast<BaseProperty**>(const_cast<const Serializeable*>(this)->Properties());
        }

        const BaseProperty* PropertyAtIndex(size_t i) const;
        BaseProperty* PropertyAtIndex(size_t i) {
            return const_cast<BaseProperty*>(const_cast<const Serializeable*>(this)->PropertyAtIndex(i));
        }

        const virtual BaseProperty* PropertyByName(const char* name) const;
        BaseProperty* PropertyByName(const char* name) {
            return const_cast<BaseProperty*>(const_cast<const Serializeable*>(this)->PropertyByName(name));
        }

        uint32_t NonCompactPropertyPosition(const char* propertyName) const;
        virtual const char* SerializeableName() const;
        bool operator==(const Serializeable& rhs) const;
        bool operator!=(const Serializeable& rhs) const;
    protected:
        uint32_t mNumberOfProperties = 0;
};

#endif
