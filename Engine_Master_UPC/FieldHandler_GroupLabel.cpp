#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IFieldContainer.h"
#include "IArchive.h"
namespace
{
    void drawGroupLabelUi(const FieldInfo& field, void*, IFieldContainer&)
    {
        ImGui::Spacing();
        ImGui::SeparatorText(field.name);
    }

    void serializeGroupLabel(const FieldInfo&, const void*, IArchive& archive)
    {
    }

    void deserializeGroupLabel(const FieldInfo&, void*, const rapidjson::Value&)
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
