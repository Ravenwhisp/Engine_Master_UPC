#include "Globals.h"

#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"
#include "IArchive.h"

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

    void serializeIntField(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        int* value = reinterpret_cast<int*>(data);
        uint32_t v = static_cast<uint32_t>(*value);
        archive.serialize(v, field.name);
        if (archive.mode() == ArchiveMode::Input)
            *value = static_cast<int>(v);
    }

    void cloneIntField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<int*>(targetData) = *reinterpret_cast<const int*>(sourceData);
    }

    void fixReferencesIntField(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler intFieldHandler = {&drawIntFieldUi, &serializeIntField, &cloneIntField, &fixReferencesIntField};
}

const ScriptFieldHandler* getIntFieldHandler()
{
    return &intFieldHandler;
}