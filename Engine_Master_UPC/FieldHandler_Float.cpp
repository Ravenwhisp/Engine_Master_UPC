#include "Globals.h"

#include "FieldInfo.h"
#include "FieldHandler.h"
#include "FieldHandlerRegistry.h"
#include "IArchive.h"
#include "IFieldContainer.h"
#include "SceneReferenceResolver.h"

namespace
{
    void drawFloatFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        float* value = reinterpret_cast<float*>(data);

        if (ImGui::DragFloat(field.name, value, field.floatInfo.dragSpeed, field.floatInfo.min, field.floatInfo.max))
        {
            container.onFieldEdited(field);
        }
    }

    void serializeFloatField(const FieldInfo& field, void* data, IArchive& archive)
    {
        float value = *reinterpret_cast<const float*>(data);
        archive.serialize(value, field.name);
        *reinterpret_cast<float*>(data) = value;
    }

    void cloneFloatField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<float*>(targetData) = *reinterpret_cast<const float*>(sourceData);
    }

    void fixReferencesFloatField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler floatFieldHandler = { &drawFloatFieldUi, &serializeFloatField, &cloneFloatField, &fixReferencesFloatField};
}

const FieldHandler* getFloatFieldHandler()
{
    return &floatFieldHandler;
}
