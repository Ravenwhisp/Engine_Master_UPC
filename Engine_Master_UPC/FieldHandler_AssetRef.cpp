#include "Globals.h"

#include "FieldInfo.h"
#include "FieldHandler.h"
#include "FieldHandlerRegistry.h"
#include "IArchive.h"
#include "IFieldContainer.h"
#include "AssetReference.h"
#include "AssetType.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "AssetIndex.h"
#include "DataContainer.h"

namespace
{
    void drawAssetRefFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        AssetReference<void>* assetRef = reinterpret_cast<AssetReference<void>*>(data);

        const bool hasAsset = assetRef->m_id.isValid();
        const char* displayName = field.name;

        ImGui::Text("%s", displayName);
        ImGui::SameLine();

        if (hasAsset)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Assigned (UID %llu)", assetRef->m_id.m_uid);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "None");
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
            {
                const UID droppedUID = *static_cast<const UID*>(payload->Data);
                AssetId* resolved = app->getModuleAssets()->findReference(droppedUID);
                if (resolved)
                {
                    assetRef->m_id = *resolved;
                    if (resolved->m_type == AssetType::DATA_CONTAINER)
                    {
                        assetRef->m_data = app->getModuleAssets()->load<DataContainer>(*resolved);
                    }
                    container.onFieldEdited(field);
                    delete resolved;
                }
            }

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB_ASSET"))
            {
                const std::string pathStr(static_cast<const char*>(payload->Data));
                const UID uid = app->getModuleAssets()->getIndex().findUID(std::filesystem::path(pathStr));
                AssetId* resolved = app->getModuleAssets()->findReference(uid);
                if (resolved)
                {
                    assetRef->m_id = *resolved;
                    container.onFieldEdited(field);
                    delete resolved;
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine();

        std::string clearLabel = std::string("Clear###") + field.name;
        if (ImGui::Button(clearLabel.c_str()))
        {
            assetRef->m_id = AssetId();
            assetRef->m_data = nullptr;
            container.onFieldEdited(field);
        }

        if (assetRef->m_data != nullptr)
        {
            auto* dc = static_cast<DataContainer*>(assetRef->m_data.get());
            ImGui::Text("%s", dc->getDisplayTypeName());

            FieldList dcFields = dc->getExposedFields();
            if (!dcFields.fields.empty())
            {
                ImGui::Spacing();

                std::string headerId = std::string(field.name) + " Settings###" + field.name + "_Settings";
                if (ImGui::CollapsingHeader(headerId.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent();

                    char* dcBase = reinterpret_cast<char*>(dc);
                    bool currentGroupOpen = true;

                    for (const FieldInfo& dcField : dcFields.fields)
                    {
                        if (dcField.type == FieldType::GroupCollapseBegin)
                        {
                            ImGui::Spacing();
                            currentGroupOpen = ImGui::CollapsingHeader(dcField.name, ImGuiTreeNodeFlags_DefaultOpen);
                            continue;
                        }

                        if (dcField.type == FieldType::GroupCollapseEnd)
                        {
                            currentGroupOpen = true;
                            continue;
                        }

                        if (!currentGroupOpen)
                            continue;

                        void* dcData = dcBase + dcField.offset;
                        assert(dcField.handler != nullptr);
                        dcField.handler->drawUi(dcField, dcData, container);
                    }

                    ImGui::Spacing();
                    if (ImGui::Button("Save"))
                    {
                        app->getModuleAssets()->save(*dc);
                    }

                    ImGui::Unindent();
                }
            }
        }
    }

    void serializeAssetRefField(const FieldInfo& field, void* data, IArchive& archive)
    {
        AssetReference<void>* ref = reinterpret_cast<AssetReference<void>*>(data);
        archive.beginObject(field.name);
        ref->m_id.serialize(archive);
        archive.endObject();
    }

    void cloneAssetRefField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        const AssetReference<void>* sourceRef = reinterpret_cast<const AssetReference<void>*>(sourceData);
        AssetReference<void>* targetRef = reinterpret_cast<AssetReference<void>*>(targetData);
        targetRef->m_id = sourceRef->m_id;
        targetRef->m_data = sourceRef->m_data;
    }

    void fixReferencesAssetRefField(const FieldInfo&, void* data, const SceneReferenceResolver&)
    {
        AssetReference<void>* assetRef = reinterpret_cast<AssetReference<void>*>(data);

        if (!assetRef->m_id.isValid() && assetRef->m_id.hasUID())
        {
            AssetId* resolved = app->getModuleAssets()->findReference(assetRef->m_id.m_uid);
            if (resolved)
            {
                assetRef->m_id = *resolved;
                if (resolved->m_type == AssetType::DATA_CONTAINER)
                {
                    assetRef->m_data = app->getModuleAssets()->load<DataContainer>(*resolved);
                }
                delete resolved;
            }
        }
    }

    const FieldHandler assetRefFieldHandler = {
        &drawAssetRefFieldUi,
        &serializeAssetRefField,
        &cloneAssetRefField,
        &fixReferencesAssetRefField
    };
}

const FieldHandler* getAssetRefFieldHandler()
{
    return &assetRefFieldHandler;
}
