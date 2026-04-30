#include "Globals.h"

#include "ScriptFieldInfo.h"
#include "ScriptFieldHandler.h"
#include "ScriptFieldHandlerRegistry.h"
#include "Script.h"
#include "ScriptComponent.h"
#include "ScriptComponentRef.h"
#include "SceneReferenceResolver.h"

#include "Application.h"
#include "ModuleScene.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"

namespace
{
    Component* findDroppedComponent(const ImGuiPayload* payload, ComponentType componentType)
    {
        GameObject* droppedObject = *(GameObject**)payload->Data;
        GameObject* sceneObject = app->getModuleScene()->getScene()->findGameObjectByUID(droppedObject->GetID());

        if (sceneObject == nullptr)
        {
            return nullptr;
        }

        if (componentType == ComponentType::TRANSFORM)
        {
            return sceneObject->GetTransform();
        }

        return sceneObject->GetComponent(componentType);
    }

    void resolveComponentReference(ScriptComponentRef<Component>& componentReference, const SceneReferenceResolver& resolver, ComponentType expectedType)
    {
        componentReference.component = nullptr;

        if (componentReference.uid == 0)
        {
            return;
        }

        Component* resolved = resolver.getClonedComponent(componentReference.uid);
        if (resolved == nullptr)
        {
            return;
        }

        if (resolved->getType() == expectedType)
        {
            componentReference.component = resolved;
        }
    }

    void drawComponentRefFieldUi(const ScriptFieldInfo& field, void* data, Script& script, ScriptComponent&)
    {
        ScriptComponentRef<Component>* componentReference = reinterpret_cast<ScriptComponentRef<Component>*>(data);

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
                Component* candidate = findDroppedComponent(payload, field.componentRefInfo.componentType);

                if (candidate != nullptr)
                {
                    componentReference->uid = candidate->getID();
                    componentReference->component = candidate;
                    script.onFieldEdited(field);
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
            script.onFieldEdited(field);
        }
    }

    void serializeComponentRefField(const ScriptFieldInfo& field, const void* data, rapidjson::Value& outFieldsJson, rapidjson::Document& domTree)
    {
        const ScriptComponentRef<Component>* componentReference = reinterpret_cast<const ScriptComponentRef<Component>*>(data);

        rapidjson::Value key(field.name, domTree.GetAllocator());
        outFieldsJson.AddMember(key, static_cast<uint64_t>(componentReference->uid), domTree.GetAllocator());
    }

    void deserializeComponentRefField(const ScriptFieldInfo&, void* data, const rapidjson::Value& valueJson)
    {
        if (!valueJson.IsUint64())
        {
            return;
        }

        ScriptComponentRef<Component>* componentReference = reinterpret_cast<ScriptComponentRef<Component>*>(data);

        componentReference->uid = static_cast<UID>(valueJson.GetUint64());
        componentReference->component = nullptr;
    }

    void cloneComponentRefField(const ScriptFieldInfo&, const void* sourceData, void* targetData)
    {
        const ScriptComponentRef<Component>* sourceRef = reinterpret_cast<const ScriptComponentRef<Component>*>(sourceData);

        ScriptComponentRef<Component>* targetRef = reinterpret_cast<ScriptComponentRef<Component>*>(targetData);

        targetRef->uid = sourceRef->uid;
        targetRef->component = nullptr;
    }

    void fixReferencesComponentRefField(const ScriptFieldInfo& field, void* data, const SceneReferenceResolver& resolver)
    {
        ScriptComponentRef<Component>* componentReference = reinterpret_cast<ScriptComponentRef<Component>*>(data);

        resolveComponentReference(*componentReference, resolver, field.componentRefInfo.componentType);
    }

    const ScriptFieldHandler componentRefFieldHandler = { &drawComponentRefFieldUi, &serializeComponentRefField, &deserializeComponentRefField, &cloneComponentRefField, &fixReferencesComponentRefField};
}

const ScriptFieldHandler* getComponentRefFieldHandler()
{
    return &componentRefFieldHandler;
}