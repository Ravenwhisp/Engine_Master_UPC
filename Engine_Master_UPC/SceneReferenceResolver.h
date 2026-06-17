#pragma once
#include<unordered_map>

#include "UID.h"

class GameObject;
class Component;

class SceneReferenceResolver
{
private:
    std::unordered_map<UID, Component*> m_componentMap;

    std::unordered_map<const GameObject*, GameObject*> m_clonedByPtr;
    std::unordered_map<UID, GameObject*> m_clonedByUID;

public:
    SceneReferenceResolver() = default;
    ~SceneReferenceResolver() = default;

    void registerComponent(UID id, Component* comp);
    void registerGameObject(const GameObject* original, GameObject* clone);
    Component* getClonedComponent(UID id) const;
    GameObject* getClonedGameObject(const GameObject* original) const;
    GameObject* getClonedGameObject(UID id) const;
};
