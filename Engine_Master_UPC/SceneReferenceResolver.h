#pragma once
#include<vector>

#include "UID.h"

class GameObject;
class Component;

class SceneReferenceResolver
{
private:
    std::vector<UID> m_componentIDs;
    std::vector<Component*> m_components;

    std::vector<const GameObject*> m_originalGOs;
    std::vector<GameObject*> m_clonedGOs;

public:
    SceneReferenceResolver();
    ~SceneReferenceResolver();

    void registerComponent(UID id, Component* comp);
    void registerGameObject(const GameObject* original, GameObject* clone);
    Component* getClonedComponent(UID id) const;
    GameObject* getClonedGameObject(const GameObject* original) const;
};
