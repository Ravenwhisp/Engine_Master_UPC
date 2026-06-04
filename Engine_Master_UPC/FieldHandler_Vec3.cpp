#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "Script.h"

namespace
{
    void drawVec3FieldUi(const FieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        Vector3* value = reinterpret_cast<Vector3*>(data);

        if (ImGui::DragFloat3(field.name, &value->x, 0.1f))
        {
            script.onFieldEdited(field);
        }
    }

    void serializeVec3Field(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const Vector3* value = reinterpret_cast<const Vector3*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        rapidjson::Value array(rapidjson::kArrayType);
        array.PushBack(value->x, domTree.GetAllocator());
        array.PushBack(value->y, domTree.GetAllocator());
        array.PushBack(value->z, domTree.GetAllocator());

        outFieldsJson.AddMember(key, array, domTree.GetAllocator());
    }

    void deserializeVec3Field(const FieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsArray() || valueJson.Size() != 3)
        {
            return;
        }

        Vector3* value = reinterpret_cast<Vector3*>(data);
        value->x = valueJson[0].GetFloat();
        value->y = valueJson[1].GetFloat();
        value->z = valueJson[2].GetFloat();
    }

    void cloneVec3Field(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<Vector3*>(targetData) = *reinterpret_cast<const Vector3*>(sourceData);
    }

    void fixReferencesVec3Field(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler vec3FieldHandler = {&drawVec3FieldUi, &serializeVec3Field, &deserializeVec3Field, &cloneVec3Field, &fixReferencesVec3Field};
}

const FieldHandler* getVec3FieldHandler()
{
    return &vec3FieldHandler;
}
