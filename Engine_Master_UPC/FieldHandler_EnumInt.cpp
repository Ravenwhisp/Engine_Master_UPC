#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "Script.h"

namespace
{
    void drawEnumIntFieldUi(const FieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        int* value = reinterpret_cast<int*>(data);

        const char* preview = "";
        if (*value >= 0 && *value < field.enumInfo.count)
        {
            preview = field.enumInfo.names[*value];
        }

        if (ImGui::BeginCombo(field.name, preview))
        {
            for (int enumIndex = 0; enumIndex < field.enumInfo.count; ++enumIndex)
            {
                bool selected = (*value == enumIndex);
                if (ImGui::Selectable(field.enumInfo.names[enumIndex], selected))
                {
                    *value = enumIndex;
                    script.onFieldEdited(field);
                }

                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    void serializeEnumIntField(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const int* value = reinterpret_cast<const int*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, *value, domTree.GetAllocator());
    }

    void deserializeEnumIntField(const FieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsInt())
        {
            return;
        }

        int* value = reinterpret_cast<int*>(data);
        *value = valueJson.GetInt();
    }

    void cloneEnumIntField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<int*>(targetData) = *reinterpret_cast<const int*>(sourceData);
    }

    void fixReferencesEnumIntField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler enumIntFieldHandler = {&drawEnumIntFieldUi, &serializeEnumIntField, &deserializeEnumIntField, &cloneEnumIntField, &fixReferencesEnumIntField};
}

const FieldHandler* getEnumIntFieldHandler()
{
    return &enumIntFieldHandler;
}
