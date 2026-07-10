#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IArchive.h"
#include "IFieldContainer.h"

namespace
{
    void drawStringFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        std::string* value = reinterpret_cast<std::string*>(data);

        char buffer[256];
        std::strncpy(buffer, value->c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        if (ImGui::InputText(field.name, buffer, sizeof(buffer)))
        {
            *value = buffer;
            container.onFieldEdited(field);
        }
    }

    void serializeStringField(const FieldInfo& field, void* data, IArchive& archive)
    {
        const std::string* value = reinterpret_cast<const std::string*>(data);
        std::string copy = *value;
        archive.serialize(copy, field.name);
    }

    void deserializeStringField(const FieldInfo& field, void* data, IArchive& archive)
    {
        archive.serialize(*reinterpret_cast<std::string*>(data), field.name);
    }

    void cloneStringField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        *reinterpret_cast<std::string*>(targetData) = *reinterpret_cast<const std::string*>(sourceData);
    }

    void fixReferencesStringField(const FieldInfo&, void*, const SceneReferenceResolver&)
    {
    }

    const FieldHandler stringFieldHandler = {&drawStringFieldUi, &serializeStringField, &cloneStringField, &fixReferencesStringField};
}

const FieldHandler* getStringFieldHandler()
{
    return &stringFieldHandler;
}
