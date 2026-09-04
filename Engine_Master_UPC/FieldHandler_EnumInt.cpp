#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IArchive.h"
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

    void serializeEnumIntField(const FieldInfo& field, void* data, IArchive& archive)
    {
        uint32_t value = static_cast<uint32_t>(*reinterpret_cast<const int*>(data));
        archive.serialize(value, field.name);
        *reinterpret_cast<int*>(data) = static_cast<int>(value);
    }

    void cloneEnumIntField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<int*>(targetData) = *reinterpret_cast<const int*>(sourceData);
    }

    void fixReferencesEnumIntField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler enumIntFieldHandler = {&drawEnumIntFieldUi, &serializeEnumIntField, &cloneEnumIntField, &fixReferencesEnumIntField};
}

const FieldHandler* getEnumIntFieldHandler()
{
    return &enumIntFieldHandler;
}
