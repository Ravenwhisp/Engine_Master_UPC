#pragma once

#include "FieldInfo.h"

class IFieldContainer
{
public:
    virtual ~IFieldContainer() = default;

    virtual FieldList getExposedFields() const
    {
        return {};
    }

    virtual void onFieldEdited(const FieldInfo& field) {}

    virtual void onAfterDeserialize() {}
};
