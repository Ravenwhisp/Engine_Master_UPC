#pragma once

#include "GameObject.h"
#include "Component.h"
#include "ScriptComponent.h"
#include "Script.h"
#include <vector>

template<typename T>
T* GameObjectAPI::getScriptOfType(GameObject* gameObject)
{
    if (gameObject == nullptr)
    {
        return nullptr;
    }

    const std::vector<Component*> components = gameObject->GetAllComponents();

    for (Component* component : components)
    {
        if (component == nullptr || component->getType() != ComponentType::SCRIPT)
        {
            continue;
        }

        ScriptComponent* scriptComponent = static_cast<ScriptComponent*>(component);
        Script* script = scriptComponent->getScript();
        if (script == nullptr)
        {
            continue;
        }

        T* typedScript = dynamic_cast<T*>(script);
        if (typedScript != nullptr)
        {
            return typedScript;
        }
    }

    return nullptr;
}

template<typename T>
const T* GameObjectAPI::getScriptOfType(const GameObject* gameObject)
{
    if (gameObject == nullptr)
    {
        return nullptr;
    }

    const std::vector<Component*> components = gameObject->GetAllComponents();

    for (Component* component : components)
    {
        if (component == nullptr || component->getType() != ComponentType::SCRIPT)
        {
            continue;
        }

        const ScriptComponent* scriptComponent = static_cast<const ScriptComponent*>(component);
        const Script* script = scriptComponent->getScript();
        if (script == nullptr)
        {
            continue;
        }

        const T* typedScript = dynamic_cast<const T*>(script);
        if (typedScript != nullptr)
        {
            return typedScript;
        }
    }

    return nullptr;
}