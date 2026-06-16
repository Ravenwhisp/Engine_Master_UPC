#include "Globals.h"

#include "FieldInfo.h"
#include "FieldHandler.h"
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
    void drawComponentRefFieldUi(const FieldInfo& field, void* data, IFieldContainer& container)
    {
        ComponentRef<Component>* componentReference = reinterpret_cast<ComponentRef<Component>*>(data);

        Component* component = componentReference->component;

        ImGui::Text("%s", field.name);
        ImGui::SameLine();

        if (component != nullptr)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", component->getOwner()->GetName().c_str());
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "None");
        }

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
                        componentReference->uid = candidate->getID();
                        componentReference->component = candidate;
                        container.onFieldEdited(field);
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::SameLine();

        std::string clearLabel = std::string("Clear###") + field.name;
        if (ImGui::Button(clearLabel.c_str()))
        {
            componentReference->uid = 0;
            componentReference->component = nullptr;
            container.onFieldEdited(field);
        }
    }

    void serializeComponentRefField(const ScriptFieldInfo& field, void* data, IArchive& archive)
    {
        ComponentRef<Component>* componentReference = reinterpret_cast<ComponentRef<Component>*>(data);
        archive.serialize(componentReference->uid, field.name);
        if (archive.mode() == ArchiveMode::Input)
            componentReference->component = nullptr;
    }

    void cloneComponentRefField(const FieldInfo&, const void* sourceData, void* targetData)
    {
        const ComponentRef<Component>* sourceRef = reinterpret_cast<const ComponentRef<Component>*>(sourceData);

        ComponentRef<Component>* targetRef = reinterpret_cast<ComponentRef<Component>*>(targetData);

        targetRef->uid = sourceRef->uid;
        targetRef->component = nullptr;
    }

    void fixReferencesComponentRefField(const FieldInfo& field, void* data, const SceneReferenceResolver& resolver)
    {
        ComponentRef<Component>* componentReference = reinterpret_cast<ComponentRef<Component>*>(data);

        componentReference->component = nullptr;

        if (componentReference->uid == 0)
        {
            return;
        }

        Component* resolved = resolver.getClonedComponent(componentReference->uid);
        if (resolved == nullptr)
        {
            return;
        }

        if (resolved->getType() == field.componentRefInfo.componentType)
        {
            componentReference->component = resolved;
        }
    }

    const ScriptFieldHandler componentRefFieldHandler = { &drawComponentRefFieldUi, &serializeComponentRefField, &cloneComponentRefField, &fixReferencesComponentRefField};
}

const FieldHandler* getComponentRefFieldHandler()
{
    return &componentRefFieldHandler;
}
