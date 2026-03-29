#include "Globals.h"
#include "SceneReferenceResolver.h"

#include "UID.h"
#include "GameObject.h"
#include "Component.h"

SceneReferenceResolver::SceneReferenceResolver()
{
    m_componentIDs.reserve(128);
    m_components.reserve(128);
    m_originalGOs.reserve(64);
    m_clonedGOs.reserve(64);
}

SceneReferenceResolver::~SceneReferenceResolver() = default;

void SceneReferenceResolver::registerGameObject(const GameObject* original, GameObject* clone)
{
    m_originalGOs.push_back(original);
    m_clonedGOs.push_back(clone);
}

void SceneReferenceResolver::registerComponent(UID id, Component* comp)
{
    for (size_t i = 0; i < m_componentIDs.size(); ++i)
    {
        if (m_componentIDs[i] == id)
            return;
    }

    m_componentIDs.push_back(id);
    m_components.push_back(comp);
}

Component* SceneReferenceResolver::getClonedComponent(UID id) const
{
    for (size_t i = 0; i < m_componentIDs.size(); ++i)
    {
        if (m_componentIDs[i] == id)
            return m_components[i];
    }
    return nullptr;
}

GameObject* SceneReferenceResolver::getClonedGameObject(const GameObject* original) const
{
    for (size_t i = 0; i < m_originalGOs.size(); ++i)
    {
        if (m_originalGOs[i] == original)
            return m_clonedGOs[i];
    }
    return nullptr;
}
