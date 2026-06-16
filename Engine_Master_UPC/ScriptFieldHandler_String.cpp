#include "Globals.h"

#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"
#include "IArchive.h"
namespace
{
    void drawStringFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
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

    void serializeStringField(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        std::string* value = reinterpret_cast<std::string*>(data);
        archive.serialize(*value, field.name);
    }

    void cloneStringField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<std::string*>(targetData) = *reinterpret_cast<const std::string*>(sourceData);
    }

    void fixReferencesStringField(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler stringFieldHandler = {&drawStringFieldUi, &serializeStringField, &cloneStringField, &fixReferencesStringField};
}

const ScriptFieldHandler* getStringFieldHandler()
{
    return &stringFieldHandler;
}