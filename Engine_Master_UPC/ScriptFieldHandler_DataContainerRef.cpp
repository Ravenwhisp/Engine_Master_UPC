#include "Globals.h"

#include "ScriptFieldInfo.h"
#include "ScriptFieldHandler.h"
#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"
#include "ScriptDataContainerRef.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "DataContainer.h"
#include "AssetReference.h"
#include "AssetType.h"

namespace
{
    void drawDataContainerRefFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        ScriptDataContainerRef* ref = reinterpret_cast<ScriptDataContainerRef*>(data);

        ImGui::Text("%s", field.name);
        ImGui::SameLine();

        if (ref->dataContainer != nullptr)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", ref->dataContainer->getDisplayTypeName());
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "None");
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
            {
                UID droppedUid = *static_cast<UID*>(payload->Data);
                AssetReference* assetRef = app->getModuleAssets()->findReference(droppedUid);

                if (assetRef != nullptr && assetRef->m_type == AssetType::DATA_CONTAINER)
                {
                    auto asset = app->getModuleAssets()->load<DataContainer>(*assetRef);
                    if (asset)
                    {
                        ref->uid = droppedUid;
                        ref->dataContainer = asset;
                        script.onFieldEdited(field);
                    }
                    delete assetRef;
                }
                else
                {
                    delete assetRef;
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine();

        std::string clearLabel = std::string("Clear###") + field.name;
        if (ImGui::Button(clearLabel.c_str()))
        {
            ref->uid = INVALID_UID;
            ref->dataContainer = nullptr;
            script.onFieldEdited(field);
        }
    }

    void serializeDataContainerRefField(const ScriptFieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const ScriptDataContainerRef* ref = reinterpret_cast<const ScriptDataContainerRef*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, static_cast<uint64_t>(ref->uid), domTree.GetAllocator());
    }

    void deserializeDataContainerRefField(const ScriptFieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsUint64())
        {
            return;
        }

        ScriptDataContainerRef* ref = reinterpret_cast<ScriptDataContainerRef*>(data);

        ref->uid = static_cast<UID>(valueJson.GetUint64());
        ref->dataContainer = nullptr;
    }

    void cloneDataContainerRefField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        const ScriptDataContainerRef* sourceRef = reinterpret_cast<const ScriptDataContainerRef*>(sourceData);

        ScriptDataContainerRef* targetRef = reinterpret_cast<ScriptDataContainerRef*>(targetData);

        targetRef->uid = sourceRef->uid;
        targetRef->dataContainer = nullptr;
    }

    void fixReferencesDataContainerRefField(const ScriptFieldInfo&, void* data, const SceneReferenceResolver&)
    {
        ScriptDataContainerRef* ref = reinterpret_cast<ScriptDataContainerRef*>(data);

        ref->dataContainer = nullptr;

        if (!isValidUID(ref->uid))
        {
            return;
        }

        AssetReference assetRef;
        assetRef.m_uid = ref->uid;
        assetRef.m_type = AssetType::DATA_CONTAINER;

        auto asset = app->getModuleAssets()->load<DataContainer>(assetRef);
        if (asset)
        {
            ref->dataContainer = asset;
        }
    }

    const ScriptFieldHandler dataContainerRefFieldHandler = {
        &drawDataContainerRefFieldUi,
        &serializeDataContainerRefField,
        &deserializeDataContainerRefField,
        &cloneDataContainerRefField,
        &fixReferencesDataContainerRefField
    };
}

const ScriptFieldHandler* getDataContainerRefFieldHandler()
{
    return &dataContainerRefFieldHandler;
}
