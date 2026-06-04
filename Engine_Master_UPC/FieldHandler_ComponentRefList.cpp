#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "Script.h"
#include "ScriptComponentRef.h"
#include "SceneReferenceResolver.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

namespace
{
    void drawComponentRefListFieldUi(const FieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        ScriptComponentRefList* componentList = reinterpret_cast<ScriptComponentRefList*>(data);

        const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
        const ImVec2 avail = ImGui::GetContentRegionAvail();

        ImGui::Text("%s", field.name);

        const float visibleRows = 3.0f;
        const float listHeight = lineHeight * (visibleRows + 1.2f);

        std::string listFrameId = std::string("##ComponentRefListFrame_") + field.name;

        int removeIndex = -1;

        if (ImGui::BeginChild(listFrameId.c_str(), ImVec2(avail.x, listHeight), true))
        {
            const float removeButtonWidth = 70.0f;

            for (size_t index = 0; index < componentList->size(); ++index)
            {
                ScriptComponentRef<Component>& entry = (*componentList)[index];
                Component* component = entry.component;

                ImGui::PushID(static_cast<int>(index));

                const float rowStartX = ImGui::GetCursorPosX();
                const float contentWidth = ImGui::GetContentRegionAvail().x;

                std::string entryLabel = "[" + std::to_string(index) + "]";
                ImGui::Text("%s", entryLabel.c_str());
                ImGui::SameLine();

                const float removePosX = rowStartX + contentWidth - removeButtonWidth;

                float nameStartX = ImGui::GetCursorPosX();
                float maxNameWidth = removePosX - nameStartX - 8.0f;
                if (maxNameWidth < 20.0f)
                {
                    maxNameWidth = 20.0f;
                }

                ImGui::BeginGroup();
                ImGui::PushTextWrapPos(nameStartX + maxNameWidth);

                if (component != nullptr)
                {
                    ImGui::TextColored(
                        ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                        "%s",
                        component->getOwner()->GetName().c_str());
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "None");
                }

                ImGui::PopTextWrapPos();
                ImGui::EndGroup();

                ImGui::SameLine(removePosX);
                if (ImGui::Button("Remove", ImVec2(removeButtonWidth, 0.0f)))
                {
                    removeIndex = static_cast<int>(index);
                }

                ImGui::PopID();
            }

            if (removeIndex >= 0)
            {
                componentList->erase(componentList->begin() + removeIndex);
                script.onFieldEdited(field);
            }
        }
        ImGui::EndChild();

        std::string clearLabel = std::string("Clear All###") + field.name;
        if (ImGui::Button(clearLabel.c_str()))
        {
            if (!componentList->empty())
            {
                componentList->clear();
                script.onFieldEdited(field);
            }
        }

        const float dropHeight = lineHeight * 1.7f;
        std::string dropFrameId = std::string("##DropFrame_") + field.name;

        if (ImGui::BeginChild(dropFrameId.c_str(), ImVec2(avail.x, dropHeight), true))
        {
            ImGui::Text("Drop GameObject here");

            ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
            std::string dropTargetId = std::string("##DropTarget_") + field.name;
            ImGui::InvisibleButton(dropTargetId.c_str(), ImGui::GetContentRegionAvail());

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
                {
                    GameObject* droppedObject = *(GameObject**)payload->Data;
                    GameObject* sceneObject = app->getModuleScene()->getScene()->findGameObjectByUID(droppedObject->GetID());

                    if (sceneObject != nullptr)
                    {
                        Component* candidate = nullptr;

                        if (field.componentRefInfo.componentType == ComponentType::TRANSFORM)
                        {
                            candidate = sceneObject->GetTransform();
                        }
                        else
                        {
                            candidate = sceneObject->GetComponent(field.componentRefInfo.componentType);
                        }

                        if (candidate != nullptr)
                        {
                            ScriptComponentRef<Component> newEntry;
                            newEntry.uid = candidate->getID();
                            newEntry.component = candidate;

                            componentList->push_back(newEntry);
                            script.onFieldEdited(field);
                        }
                    }
                }

                ImGui::EndDragDropTarget();
            }
        }
        ImGui::EndChild();
    }

    void serializeComponentRefListField(const FieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const ScriptComponentRefList* componentList = reinterpret_cast<const ScriptComponentRefList*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        rapidjson::Value array(rapidjson::kArrayType);

        for (const ScriptComponentRef<Component>& entry : *componentList)
        {
            array.PushBack(static_cast<uint64_t>(entry.uid), domTree.GetAllocator());
        }

        outFieldsJson.AddMember(key, array, domTree.GetAllocator());
    }

    void deserializeComponentRefListField(const FieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsArray())
        {
            return;
        }

        ScriptComponentRefList* componentList = reinterpret_cast<ScriptComponentRefList*>(data);
        componentList->clear();

        for (rapidjson::SizeType i = 0; i < valueJson.Size(); ++i)
        {
            if (!valueJson[i].IsUint64())
            {
                continue;
            }

            ScriptComponentRef<Component> entry;
            entry.uid = static_cast<UID>(valueJson[i].GetUint64());
            entry.component = nullptr;

            componentList->push_back(entry);
        }
    }

    void cloneComponentRefListField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        const ScriptComponentRefList* sourceList = reinterpret_cast<const ScriptComponentRefList*>(sourceData);
        ScriptComponentRefList* targetList = reinterpret_cast<ScriptComponentRefList*>(targetData);

        targetList->clear();
        targetList->reserve(sourceList->size());

        for (const ScriptComponentRef<Component>& sourceEntry : *sourceList)
        {
            ScriptComponentRef<Component> targetEntry;
            targetEntry.uid = sourceEntry.uid;
            targetEntry.component = nullptr;
            targetList->push_back(targetEntry);
        }
    }

    void fixReferencesComponentRefListField(const FieldInfo& field, void* data, const SceneReferenceResolver& resolver)
    {
        ScriptComponentRefList* componentList = reinterpret_cast<ScriptComponentRefList*>(data);

        for (ScriptComponentRef<Component>& entry : *componentList)
        {
            entry.component = nullptr;

            if (entry.uid == 0)
            {
                continue;
            }

            Component* resolved = resolver.getClonedComponent(entry.uid);
            if (resolved == nullptr)
            {
                continue;
            }

            if (resolved->getType() == field.componentRefInfo.componentType)
            {
                entry.component = resolved;
            }
        }
    }

    const FieldHandler componentRefListFieldHandler = {&drawComponentRefListFieldUi, &serializeComponentRefListField, &deserializeComponentRefListField, &cloneComponentRefListField, &fixReferencesComponentRefListField};
}

const FieldHandler* getComponentRefListFieldHandler()
{
    return &componentRefListFieldHandler;
}
