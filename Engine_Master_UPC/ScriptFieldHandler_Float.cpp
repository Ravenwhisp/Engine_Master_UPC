#include "Globals.h"

#include "ScriptFieldInfo.h"
#include "ScriptFieldHandler.h"
#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"
#include "ScriptComponent.h"
#include "SceneReferenceResolver.h"

namespace
{
    void drawFloatFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        float* value = reinterpret_cast<float*>(data);

        if (ImGui::DragFloat(field.name, value, field.floatInfo.dragSpeed, field.floatInfo.min, field.floatInfo.max))
        {
            script.onFieldEdited(field);
        }
    }

    void serializeFloatField(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        float* value = reinterpret_cast<float*>(data);
        archive.serialize(*value, field.name);
    }

    void cloneFloatField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<float*>(targetData) = *reinterpret_cast<const float*>(sourceData);
    }

    void fixReferencesFloatField(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler floatFieldHandler = { &drawFloatFieldUi, &serializeFloatField, &cloneFloatField, &fixReferencesFloatField};
}

const ScriptFieldHandler* getFloatFieldHandler()
{
    return &floatFieldHandler;
}