#include "BaseProperty.hpp"

bool BaseProperty::PropertyIsPrimitive(property_t p)
{
    return p & PRIMITIVE_PROPERTY_MASK;
}

bool BaseProperty::PropertyIsBuffer(property_t p)
{
    return p & BUFFER_PROPERTY_MASK;
}

bool BaseProperty::PropertyIsSet(property_t p)
{
    return p & SET_PROPERTY_MASK;
}

BaseProperty::PropertyType BaseProperty::Type() const  {
    return mType;
};

char* BaseProperty::Name() const {
    return (char*) mName;
};

