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

    void serializeGroupLabel(const FieldInfo&, const void*, IArchive&)
    {
    }

    void deserializeGroupLabel(const FieldInfo&, void*, IArchive&)
    {
    }

    void cloneGroupLabel(const FieldInfo&, const void*, void*)
    {
    }

    void fixReferencesGroupLabel(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler groupLabelFieldHandler = {&drawGroupLabelUi, &serializeGroupLabel, &deserializeGroupLabel, &cloneGroupLabel, &fixReferencesGroupLabel};
}

const FieldHandler* getGroupLabelFieldHandler()
{
    return &groupLabelFieldHandler;
}
