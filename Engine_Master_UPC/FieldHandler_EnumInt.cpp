#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IFieldContainer.h"

namespace
{
    void drawEnumIntFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
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
                    container.onFieldEdited(field);
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

    void cloneEnumIntField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<int*>(targetData) = *reinterpret_cast<const int*>(sourceData);
    }

    void fixReferencesEnumIntField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler enumIntFieldHandler = {&drawEnumIntFieldUi, &serializeEnumIntField, &cloneEnumIntField, &fixReferencesEnumIntField};
}

const FieldHandler* getEnumIntFieldHandler()
{
    return &enumIntFieldHandler;
}
