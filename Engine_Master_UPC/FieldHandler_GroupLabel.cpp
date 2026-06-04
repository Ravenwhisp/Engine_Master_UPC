#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IFieldContainer.h"

namespace
{
    void drawGroupLabelUi(const FieldInfo& field, void*, IFieldContainer&)
    {
        ImGui::Spacing();
        ImGui::SeparatorText(field.name);
    }

    void serializeGroupLabel(const FieldInfo&, const void*, rapidjson::Value&, rapidjson::Document&)
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

    const FieldHandler groupLabelFieldHandler = {&drawGroupLabelUi, &serializeGroupLabel, &deserializeGroupLabel, &cloneGroupLabel, &fixReferencesGroupLabel};
}

const FieldHandler* getGroupLabelFieldHandler()
{
    return &groupLabelFieldHandler;
}
