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

    void serializeGroupLabel(const ScriptFieldInfo&, void*, IArchive&)
    {
    }

    void cloneGroupLabel(const ScriptFieldInfo&, const void*, void*)
    {
    }

    void fixReferencesGroupLabel(const ScriptFieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const ScriptFieldHandler groupLabelFieldHandler = {&drawGroupLabelUi, &serializeGroupLabel, &cloneGroupLabel, &fixReferencesGroupLabel};
}

const ScriptFieldHandler* getGroupLabelFieldHandler()
{
    return &groupLabelFieldHandler;
}