#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IFieldContainer.h"

namespace
{
    void drawBoolFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        bool* value = reinterpret_cast<bool*>(data);

        if (ImGui::Checkbox(field.name, value))
        {
            container.onFieldEdited(field);
        }
    }

    void serializeBoolField(const FieldInfo& field, void* data, IArchive& archive)
    {
        bool* value = reinterpret_cast<bool*>(data);
        archive.serialize(*value, field.name);
    }

    void cloneBoolField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<bool*>(targetData) = *reinterpret_cast<const bool*>(sourceData);
    }

    void fixReferencesBoolField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler boolFieldHandler = { &drawBoolFieldUi, &serializeBoolField, &cloneBoolField, &fixReferencesBoolField};
}

const FieldHandler* getBoolFieldHandler()
{
    return &boolFieldHandler;
}
