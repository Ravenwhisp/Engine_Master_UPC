#include "Globals.h"
#include "FieldUtils.h"
#include "IFieldContainer.h"
#include "FieldInfo.h"
#include "SceneReferenceResolver.h"

namespace FieldUtils
{
    void drawUi(IFieldContainer& container, char* base)
    {
        FieldList fieldList = container.getExposedFields();
        bool currentGroupOpen = true;

        for (const FieldInfo& field : fieldList.fields)
        {
            if (field.type == FieldType::GroupCollapseBegin)
            {
                ImGui::Spacing();
                currentGroupOpen = ImGui::CollapsingHeader(field.name, ImGuiTreeNodeFlags_DefaultOpen);
                continue;
            }

            if (field.type == FieldType::GroupCollapseEnd)
            {
                currentGroupOpen = true;
                continue;
            }

            if (!currentGroupOpen)
                continue;

            void* data = base + field.offset;
            assert(field.handler != nullptr);
            field.handler->drawUi(field, data, container);
        }
    }

    void serialize(const IFieldContainer& container, const char* base, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        FieldList fieldList = container.getExposedFields();

        for (const FieldInfo& field : fieldList.fields)
        {
            if (!field.isDataField())
                continue;

            const void* data = base + field.offset;
            assert(field.handler != nullptr);
            field.handler->serialize(field, data, outFieldsJson, domTree);
        }
    }

    void deserialize(IFieldContainer& container, char* base, const rapidjson::Value& fieldsJson)
    {
        FieldList fieldList = container.getExposedFields();

        for (const FieldInfo& field : fieldList.fields)
        {
            if (!field.isDataField())
                continue;

            if (!fieldsJson.HasMember(field.name))
                continue;

            void* data = base + field.offset;
            const rapidjson::Value& valueJson = fieldsJson[field.name];
            assert(field.handler != nullptr);
            field.handler->deserialize(field, data, valueJson);
        }
    }

    void clone(const IFieldContainer& source, const char* srcBase, IFieldContainer& target, char* dstBase)
    {
        FieldList sourceFields = source.getExposedFields();
        FieldList targetFields = target.getExposedFields();

        const size_t count = std::min(sourceFields.fields.size(), targetFields.fields.size());

        for (size_t i = 0; i < count; ++i)
        {
            const FieldInfo& sourceField = sourceFields.fields[i];
            const FieldInfo& targetField = targetFields.fields[i];

            if (!sourceField.isDataField() || !targetField.isDataField())
                continue;

            assert(sourceField.handler == targetField.handler);

            const void* sourceData = srcBase + sourceField.offset;
            void* targetData = dstBase + targetField.offset;

            assert(sourceField.handler != nullptr);
            sourceField.handler->clone(sourceField, sourceData, targetData);
        }
    }

    void fixReferences(IFieldContainer& container, char* base, const SceneReferenceResolver& resolver)
    {
        FieldList fieldList = container.getExposedFields();

        for (const FieldInfo& field : fieldList.fields)
        {
            if (!field.isDataField())
                continue;

            void* data = base + field.offset;
            assert(field.handler != nullptr);
            field.handler->fixReferences(field, data, resolver);
        }
    }
}
