#include "Globals.h"

#include "FieldHandlerRegistry.h"
#include "IFieldContainer.h"
#include "ComponentRef.h"
#include "SceneReferenceResolver.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

namespace
{
    void drawComponentRefListFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        ComponentRefList* componentList = reinterpret_cast<ComponentRefList*>(data);

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
                ComponentRef<Component>& entry = (*componentList)[index];
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
                container.onFieldEdited(field);
            }
        }
        ImGui::EndChild();

        std::string clearLabel = std::string("Clear All###") + field.name;
        if (ImGui::Button(clearLabel.c_str()))
        {
            if (!componentList->empty())
            {
                componentList->clear();
                container.onFieldEdited(field);
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
                            ComponentRef<Component> newEntry;
                            newEntry.uid = candidate->getID();
                            newEntry.component = candidate;

                            componentList->push_back(newEntry);
                            container.onFieldEdited(field);
                        }
                    }
                }

                ImGui::EndDragDropTarget();
            }
        }
        ImGui::EndChild();
    }

    void serializeComponentRefListField(const FieldHandler& field, void* data, IArchive& archive)
    {
        ComponentRefList* componentList = reinterpret_cast<ComponentRefList*>(data);
        uint32_t count = archive.mode() == ArchiveMode::Output ? static_cast<uint32_t>(componentList->size()) : 0;
        archive.beginArray(count, field.name);

        if (archive.mode() == ArchiveMode::Input)
        {
            componentList->clear();
            componentList->reserve(count);
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            if (archive.mode() == ArchiveMode::Output)
            {
                uint64_t uid = static_cast<uint64_t>((*componentList)[i].uid);
                archive.serialize(uid, "");
            }
            else
            {
                ComponentRef<Component> entry;
                archive.serialize(entry.uid, "");
                entry.component = nullptr;
                componentList->push_back(std::move(entry));
            }
        }

        archive.endArray();
    }

    void cloneComponentRefListField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        const ComponentRefList* sourceList = reinterpret_cast<const ComponentRefList*>(sourceData);
        ComponentRefList* targetList = reinterpret_cast<ComponentRefList*>(targetData);

        targetList->clear();
        targetList->reserve(sourceList->size());

        for (const ComponentRef<Component>& sourceEntry : *sourceList)
        {
            ComponentRef<Component> targetEntry;
            targetEntry.uid = sourceEntry.uid;
            targetEntry.component = nullptr;
            targetList->push_back(targetEntry);
        }
    }

    void fixReferencesComponentRefListField(const FieldInfo& field, void* data, const SceneReferenceResolver& resolver)
    {
        ComponentRefList* componentList = reinterpret_cast<ComponentRefList*>(data);

        for (ComponentRef<Component>& entry : *componentList)
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

    const FieldHandler componentRefListFieldHandler = {&drawComponentRefListFieldUi, &serializeComponentRefListField, &cloneComponentRefListField, &fixReferencesComponentRefListField};
}

const FieldHandler* getComponentRefListFieldHandler()
{
    return &componentRefListFieldHandler;
}
