#include "Globals.h"

#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"

namespace
{
    void drawGroupLabelUi(const ScriptFieldInfo& field, void*, Script&, ScriptComponent&)
    {
        ImGui::Spacing();
        ImGui::SeparatorText(field.name);
    }

    void serializeGroupLabel(const ScriptFieldInfo&, const void*, rapidjson::Value&, rapidjson::Document&)
    {
    }

    void deserializeGroupLabel(const ScriptFieldInfo&, void*, const rapidjson::Value&)
    {
    }

    void cloneGroupLabel(const ScriptFieldInfo&, const void*, void*)
    {
    }

    void fixReferencesGroupLabel(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler groupLabelFieldHandler = {&drawGroupLabelUi, &serializeGroupLabel, &deserializeGroupLabel, &cloneGroupLabel, &fixReferencesGroupLabel};
}

const ScriptFieldHandler* getGroupLabelFieldHandler()
{
    return &groupLabelFieldHandler;
}