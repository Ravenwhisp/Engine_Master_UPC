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

    void serializeBoolField(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const bool* value = reinterpret_cast<const bool*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, *value, domTree.GetAllocator());
    }

    void deserializeBoolField(const FieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsBool())
        {
            return;
        }

        bool* value = reinterpret_cast<bool*>(data);
        *value = valueJson.GetBool();
    }

    void cloneBoolField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<bool*>(targetData) = *reinterpret_cast<const bool*>(sourceData);
    }

    void fixReferencesBoolField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler boolFieldHandler ={&drawBoolFieldUi, &serializeBoolField, &deserializeBoolField, &cloneBoolField, &fixReferencesBoolField};
}

const FieldHandler* getBoolFieldHandler()
{
    return &boolFieldHandler;
}
