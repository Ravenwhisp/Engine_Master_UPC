#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IFieldContainer.h"

namespace
{
    void drawIntFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        int* value = reinterpret_cast<int*>(data);

        if (ImGui::DragInt(field.name, value))
        {
            container.onFieldEdited(field);
        }
    }

    void serializeIntField(const FieldInfo& field, void* data, IArchive& archive)
    {
        int* value = reinterpret_cast<int*>(data);
        uint32_t temp = static_cast<uint32_t>(*value);
        archive.serialize(temp, field.name);
        if (archive.mode() == ArchiveMode::Input)
            *value = static_cast<int>(temp);
    }

    void cloneIntField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<int*>(targetData) = *reinterpret_cast<const int*>(sourceData);
    }

    void fixReferencesIntField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler intFieldHandler = { &drawIntFieldUi, &serializeIntField, &cloneIntField, &fixReferencesIntField};
}

const FieldHandler* getIntFieldHandler()
{
    return &intFieldHandler;
}
