#include "Globals.h"

#include "ScriptFieldInfo.h"
#include "ScriptFieldHandler.h"
#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"
#include "AssetRef.h"
#include "AssetReference.h"
#include "AssetType.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "AssetIndex.h"

namespace
{
    void drawAssetRefFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
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
                    if (field.assetRefInfo.assetType == AssetType::UNKNOWN ||
                        resolved->m_type == field.assetRefInfo.assetType)
                    {
                        assetRef->m_ref = *resolved;
                        script.onFieldEdited(field);
                    }
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
                    if (field.assetRefInfo.assetType == AssetType::UNKNOWN ||
                        resolved->m_type == field.assetRefInfo.assetType)
                    {
                        assetRef->m_ref = *resolved;
                        script.onFieldEdited(field);
                    }
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
            script.onFieldEdited(field);
        }
    }

    void serializeAssetRefField(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        AssetRef<void>* assetRef = reinterpret_cast<AssetRef<void>*>(data);
        assetRef->m_ref.serialize(archive);
    }

    void cloneAssetRefField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        const AssetRef<void>* sourceRef = reinterpret_cast<const AssetRef<void>*>(sourceData);
        AssetRef<void>* targetRef = reinterpret_cast<AssetRef<void>*>(targetData);
        targetRef->m_ref = sourceRef->m_ref;
    }

    void fixReferencesAssetRefField(const ScriptFieldInfo&, void* data, const SceneReferenceResolver&)
    {
        AssetRef<void>* assetRef = reinterpret_cast<AssetRef<void>*>(data);

        if (!assetRef->m_ref.isValid() && assetRef->m_ref.hasUID())
        {
            AssetReference* resolved = app->getModuleAssets()->findReference(assetRef->m_ref.m_uid);
            if (resolved)
            {
                assetRef->m_ref = *resolved;
                delete resolved;
            }
        }
    }

    const ScriptFieldHandler assetRefFieldHandler = {
        &drawAssetRefFieldUi,
        &serializeAssetRefField,
        &cloneAssetRefField,
        &fixReferencesAssetRefField
    };
}

const ScriptFieldHandler* getAssetRefFieldHandler()
{
    return &assetRefFieldHandler;
}
