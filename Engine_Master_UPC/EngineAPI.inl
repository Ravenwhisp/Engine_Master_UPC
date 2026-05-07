#pragma once

#include "GameObject.h"
#include "Component.h"
#include "Script.h"
#include <vector>

template<typename T>
T* GameObjectAPI::findScript(GameObject* gameObject)
{
    if (gameObject == nullptr)
    {
        return nullptr;
    }

    const int scriptCount = GameObjectAPI::getScriptCount(gameObject);

    for (int i = 0; i < scriptCount; ++i)
    {
        Script* script = GameObjectAPI::getScriptByIndex(gameObject, i);

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
const T* GameObjectAPI::findScript(const GameObject* gameObject)
{
    if (gameObject == nullptr)
    {
        return nullptr;
    }

    const int scriptCount = GameObjectAPI::getScriptCount(gameObject);

    for (int i = 0; i < scriptCount; ++i)
    {
        const Script* script = GameObjectAPI::getScriptByIndex(gameObject, i);

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