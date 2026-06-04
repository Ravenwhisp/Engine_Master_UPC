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

    void serializeIntField(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const int* value = reinterpret_cast<const int*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, *value, domTree.GetAllocator());
    }

    void deserializeIntField(const FieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsInt())
        {
            return;
        }

        int* value = reinterpret_cast<int*>(data);
        *value = valueJson.GetInt();
    }

    void cloneIntField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<int*>(targetData) = *reinterpret_cast<const int*>(sourceData);
    }

    void fixReferencesIntField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler intFieldHandler = {&drawIntFieldUi, &serializeIntField, &deserializeIntField, &cloneIntField, &fixReferencesIntField};
}

const FieldHandler* getIntFieldHandler()
{
    return &intFieldHandler;
}
