#include "Globals.h"

#include "FieldInfo.h"
#include "FieldHandler.h"
#include "FieldHandlerRegistry.h"
#include "Script.h"
#include "ScriptComponent.h"
#include "SceneReferenceResolver.h"

namespace
{
    void drawFloatFieldUi(const FieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        float* value = reinterpret_cast<float*>(data);

        if (ImGui::DragFloat(field.name, value, field.floatInfo.dragSpeed, field.floatInfo.min, field.floatInfo.max))
        {
            script.onFieldEdited(field);
        }
    }

    void serializeFloatField(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const float* value = reinterpret_cast<const float*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, *value, domTree.GetAllocator());
    }

    void deserializeFloatField(const FieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsNumber())
        {
            return;
        }

        float* value = reinterpret_cast<float*>(data);
        *value = valueJson.GetFloat();
    }

    void cloneFloatField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<float*>(targetData) = *reinterpret_cast<const float*>(sourceData);
    }

    void fixReferencesFloatField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler floatFieldHandler = { &drawFloatFieldUi, &serializeFloatField, &deserializeFloatField, &cloneFloatField, &fixReferencesFloatField};
}

const FieldHandler* getFloatFieldHandler()
{
    return &floatFieldHandler;
}
