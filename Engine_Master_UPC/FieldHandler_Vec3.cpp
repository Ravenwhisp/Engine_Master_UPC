#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IArchive.h"
#include "IFieldContainer.h"

namespace
{
    void drawVec3FieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        Vector3* value = reinterpret_cast<Vector3*>(data);

        if (ImGui::DragFloat3(field.name, &value->x, 0.1f))
        {
            container.onFieldEdited(field);
        }
    }

    void serializeVec3Field(const FieldInfo& field, void* data, IArchive& archive)
    {
        Vector3 value = *reinterpret_cast<const Vector3*>(data);
        archive.serialize(value, field.name);
    }

    void deserializeVec3Field(const FieldInfo& field, void* data, IArchive& archive)
    {
        archive.serialize(*reinterpret_cast<Vector3*>(data), field.name);
    }

    void cloneVec3Field(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<Vector3*>(targetData) = *reinterpret_cast<const Vector3*>(sourceData);
    }

    void fixReferencesVec3Field(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler vec3FieldHandler = {&drawVec3FieldUi, &serializeVec3Field, &cloneVec3Field, &fixReferencesVec3Field};
}

const FieldHandler* getVec3FieldHandler()
{
    return &vec3FieldHandler;
}
