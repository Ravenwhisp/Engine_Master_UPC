#include "Globals.h"

#include "FieldInfo.h"
#include "FieldHandler.h"
#include "FieldHandlerRegistry.h"
#include "Script.h"
#include "ScriptDataContainerRef.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "DataContainer.h"
#include "AssetReference.h"
#include "AssetType.h"

namespace
{
    void drawDataContainerRefFieldUi(const FieldInfo& field, void* data, Script& script, ScriptComponent& owner)
    {
        ScriptDataContainerRef<DataContainer>* ref = reinterpret_cast<ScriptDataContainerRef<DataContainer>*>(data);

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

        if (ref->dataContainer != nullptr)
        {
            FieldList dcFields = ref->dataContainer->getExposedFields();
            if (!dcFields.fields.empty())
            {
                ImGui::Spacing();

                std::string headerId = std::string(field.name) + " Settings###" + field.name + "_Settings";
                if (ImGui::CollapsingHeader(headerId.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent();

                    char* dcBase = reinterpret_cast<char*>(ref->dataContainer.get());
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
                        dcField.handler->drawUi(dcField, dcData, script, owner);
                    }

                    ImGui::Spacing();
                    if (ImGui::Button("Save"))
                    {
                        app->getModuleAssets()->save(*ref->dataContainer);
                    }

                    ImGui::Unindent();
                }
            }
        }
    }

    void serializeDataContainerRefField(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const ScriptDataContainerRef<DataContainer>* ref = reinterpret_cast<const ScriptDataContainerRef<DataContainer>*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, static_cast<uint64_t>(ref->uid), domTree.GetAllocator());
    }

    void deserializeDataContainerRefField(const FieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsUint64())
        {
            return;
        }

        ScriptDataContainerRef<DataContainer>* ref = reinterpret_cast<ScriptDataContainerRef<DataContainer>*>(data);

        ref->uid = static_cast<UID>(valueJson.GetUint64());
        ref->dataContainer = nullptr;
    }

    void cloneDataContainerRefField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        const ScriptDataContainerRef<DataContainer>* sourceRef = reinterpret_cast<const ScriptDataContainerRef<DataContainer>*>(sourceData);

        ScriptDataContainerRef<DataContainer>* targetRef = reinterpret_cast<ScriptDataContainerRef<DataContainer>*>(targetData);

        targetRef->uid = sourceRef->uid;
        targetRef->dataContainer = nullptr;
    }

    void fixReferencesDataContainerRefField(const FieldInfo&, void* data, const SceneReferenceResolver&)
    {
        ScriptDataContainerRef<DataContainer>* ref = reinterpret_cast<ScriptDataContainerRef<DataContainer>*>(data);

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

    const FieldHandler dataContainerRefFieldHandler = {
        &drawDataContainerRefFieldUi,
        &serializeDataContainerRefField,
        &deserializeDataContainerRefField,
        &cloneDataContainerRefField,
        &fixReferencesDataContainerRefField
    };
}

const FieldHandler* getDataContainerRefFieldHandler()
{
    return &dataContainerRefFieldHandler;
}
