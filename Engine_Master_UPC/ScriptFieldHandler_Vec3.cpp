#include "Globals.h"

#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"

namespace
{
    void drawVec3FieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        Vector3* value = reinterpret_cast<Vector3*>(data);

        if (ImGui::DragFloat3(field.name, &value->x, 0.1f))
        {
            script.onFieldEdited(field);
        }
    }

    void serializeVec3Field(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        Vector3* value = reinterpret_cast<Vector3*>(data);
        DirectX::SimpleMath::Vector3 v(value->x, value->y, value->z);
        archive.serialize(v, field.name);
        if (archive.mode() == ArchiveMode::Input)
        {
            value->x = v.x; value->y = v.y; value->z = v.z;
        }
    }

    void cloneVec3Field(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<Vector3*>(targetData) = *reinterpret_cast<const Vector3*>(sourceData);
    }

    void fixReferencesVec3Field(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler vec3FieldHandler = {&drawVec3FieldUi, &serializeVec3Field, &cloneVec3Field, &fixReferencesVec3Field};
}

const ScriptFieldHandler* getVec3FieldHandler()
{
    return &vec3FieldHandler;
}