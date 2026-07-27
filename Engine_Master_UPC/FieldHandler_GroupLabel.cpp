#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IArchive.h"
#include "IFieldContainer.h"

namespace
{
    void drawGroupLabelUi(const FieldInfo& field, void*, IFieldContainer&)
    {
        ImGui::Spacing();
        ImGui::SeparatorText(field.name);
    }

    void serializeGroupLabel(const FieldInfo&, void*, IArchive&)
    {
    }

    void cloneGroupLabel(const FieldInfo&, const void*, void*)
    {
    }

    void fixReferencesGroupLabel(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler groupLabelFieldHandler = {&drawGroupLabelUi, &serializeGroupLabel, &cloneGroupLabel, &fixReferencesGroupLabel};
}

const FieldHandler* getGroupLabelFieldHandler()
{
    return &groupLabelFieldHandler;
}
