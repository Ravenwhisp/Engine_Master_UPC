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

    void serialize(IFieldContainer& container, char* base, IArchive& archive)
    {
        FieldList fieldList = container.getExposedFields();

        for (const FieldInfo& field : fieldList.fields)
        {
            if (!field.isDataField())
                continue;

            void* data = base + field.offset;
            assert(field.handler != nullptr);
            field.handler->serialize(field, data, archive);
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
