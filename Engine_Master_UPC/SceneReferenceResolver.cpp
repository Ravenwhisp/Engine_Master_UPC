#include "Globals.h"
#include "SceneReferenceResolver.h"

#include "UID.h"
#include "GameObject.h"
#include "Component.h"

void SceneReferenceResolver::registerGameObject(const GameObject* original, GameObject* clone)
{
    m_clonedByUID[original->GetID()] = clone;
    m_clonedByPtr[original] = clone;
}

void SceneReferenceResolver::registerComponent(UID id, Component* comp)
{
    m_componentMap.try_emplace(id, comp);
}

void SceneReferenceResolver::mergeFrom(const SceneReferenceResolver& other)
{
    m_componentMap.insert(other.m_componentMap.begin(), other.m_componentMap.end());
    m_clonedByPtr.insert(other.m_clonedByPtr.begin(), other.m_clonedByPtr.end());
    m_clonedByUID.insert(other.m_clonedByUID.begin(), other.m_clonedByUID.end());
}

Component* SceneReferenceResolver::getClonedComponent(UID id) const
{
    auto it = m_componentMap.find(id);
    return it != m_componentMap.end() ? it->second : nullptr;
}

GameObject* SceneReferenceResolver::getClonedGameObject(const GameObject* original) const
{
    auto it = m_clonedByPtr.find(original);
    return it != m_clonedByPtr.end() ? it->second : nullptr;
}

GameObject* SceneReferenceResolver::getClonedGameObject(UID id) const
{
    auto it = m_clonedByUID.find(id);
    return it != m_clonedByUID.end() ? it->second : nullptr;
}