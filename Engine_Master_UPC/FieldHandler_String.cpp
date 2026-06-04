#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "Script.h"

namespace
{
    void drawStringFieldUi(const FieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        std::string* value = reinterpret_cast<std::string*>(data);

        char buffer[256];
        std::strncpy(buffer, value->c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        if (ImGui::InputText(field.name, buffer, sizeof(buffer)))
        {
            *value = buffer;
            script.onFieldEdited(field);
        }
    }

    void serializeStringField(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const std::string* value = reinterpret_cast<const std::string*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, rapidjson::Value(value->c_str(), domTree.GetAllocator()), domTree.GetAllocator());
    }

    void deserializeStringField(const FieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsString())
        {
            return;
        }

        std::string* value = reinterpret_cast<std::string*>(data);
        *value = valueJson.GetString();
    }

    void cloneStringField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<std::string*>(targetData) = *reinterpret_cast<const std::string*>(sourceData);
    }

    void fixReferencesStringField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler stringFieldHandler = {&drawStringFieldUi, &serializeStringField, &deserializeStringField, &cloneStringField, &fixReferencesStringField};
}

const FieldHandler* getStringFieldHandler()
{
    return &stringFieldHandler;
}
