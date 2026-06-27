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
