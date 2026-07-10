#include "Globals.h"

#include "FieldInfo.h"
#include "FieldHandler.h"
#include "FieldHandlerRegistry.h"
#include "IArchive.h"
#include "IFieldContainer.h"
#include "AssetRef.h"
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
        AssetRef<void>* assetRef = reinterpret_cast<AssetRef<void>*>(data);

        const bool hasAsset = assetRef->m_ref.isValid();
        const char* displayName = field.name;

        ImGui::Text("%s", displayName);
        ImGui::SameLine();

        if (hasAsset)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Assigned (UID %llu)", assetRef->m_ref.m_uid);
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
                AssetReference* resolved = app->getModuleAssets()->findReference(droppedUID);
                if (resolved)
                {
                    assetRef->m_ref = *resolved;
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
                AssetReference* resolved = app->getModuleAssets()->findReference(uid);
                if (resolved)
                {
                    assetRef->m_ref = *resolved;
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
            assetRef->m_ref = AssetReference();
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
        AssetRef<void>* ref = reinterpret_cast<AssetRef<void>*>(data);
        uint64_t uid = static_cast<uint64_t>(ref->m_ref.m_uid);
        archive.serialize(uid, field.name);
        ref->m_ref.m_uid = static_cast<UID>(uid);

        uint32_t type = static_cast<uint32_t>(ref->m_ref.m_type);
        std::string typeKey = std::string(field.name) + "_type";
        archive.serializeStringEnum(type, typeKey.c_str(), AssetTypeToString, StringToAssetType);
        ref->m_ref.m_type = static_cast<AssetType>(type);
    }

    void cloneAssetRefField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        const AssetRef<void>* sourceRef = reinterpret_cast<const AssetRef<void>*>(sourceData);
        AssetRef<void>* targetRef = reinterpret_cast<AssetRef<void>*>(targetData);
        targetRef->m_ref = sourceRef->m_ref;
        targetRef->m_data = sourceRef->m_data;
    }

    void fixReferencesAssetRefField(const FieldInfo&, void* data, const SceneReferenceResolver&)
    {
        AssetRef<void>* assetRef = reinterpret_cast<AssetRef<void>*>(data);

        if (!assetRef->m_ref.isValid() && assetRef->m_ref.hasUID())
        {
            AssetReference* resolved = app->getModuleAssets()->findReference(assetRef->m_ref.m_uid);
            if (resolved)
            {
                assetRef->m_ref = *resolved;
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
