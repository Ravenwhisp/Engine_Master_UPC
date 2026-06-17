#include "Globals.h"

#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"
#include "IArchive.h"

namespace
{
    void drawEnumIntFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
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

    void serializeEnumIntField(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        int* value = reinterpret_cast<int*>(data);
        uint32_t v = static_cast<uint32_t>(*value);
        archive.serialize(v, field.name);
        if (archive.mode() == ArchiveMode::Input)
            *value = static_cast<int>(v);
    }

    void cloneEnumIntField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<int*>(targetData) = *reinterpret_cast<const int*>(sourceData);
    }

    void fixReferencesEnumIntField(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler enumIntFieldHandler = {&drawEnumIntFieldUi, &serializeEnumIntField, &cloneEnumIntField, &fixReferencesEnumIntField};
}

const ScriptFieldHandler* getEnumIntFieldHandler()
{
    return &enumIntFieldHandler;
}