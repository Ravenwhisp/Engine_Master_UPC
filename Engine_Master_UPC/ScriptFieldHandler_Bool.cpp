#include "Globals.h"

#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"

namespace
{
    void drawBoolFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        bool* value = reinterpret_cast<bool*>(data);

        if (ImGui::Checkbox(field.name, value))
        {
            script.onFieldEdited(field);
        }
    }

    void serializeBoolField(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        bool* value = reinterpret_cast<bool*>(data);
        archive.serialize(*value, field.name);
    }

    void cloneBoolField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<bool*>(targetData) = *reinterpret_cast<const bool*>(sourceData);
    }

    void fixReferencesBoolField(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler boolFieldHandler ={&drawBoolFieldUi, &serializeBoolField, &cloneBoolField, &fixReferencesBoolField};
}

const ScriptFieldHandler* getBoolFieldHandler()
{
    return &boolFieldHandler;
}