#include "Globals.h"

#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"

namespace
{
    void drawIntFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        int* value = reinterpret_cast<int*>(data);

        if (ImGui::DragInt(field.name, value))
        {
            script.onFieldEdited(field);
        }
    }

    void serializeIntField(const ScriptFieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const int* value = reinterpret_cast<const int*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, *value, domTree.GetAllocator());
    }

    void deserializeIntField(const ScriptFieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsInt())
        {
            return;
        }

        int* value = reinterpret_cast<int*>(data);
        *value = valueJson.GetInt();
    }

    void cloneIntField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<int*>(targetData) = *reinterpret_cast<const int*>(sourceData);
    }

    void fixReferencesIntField(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler intFieldHandler = {&drawIntFieldUi, &serializeIntField, &deserializeIntField, &cloneIntField, &fixReferencesIntField};
}

const ScriptFieldHandler* getIntFieldHandler()
{
    return &intFieldHandler;
}