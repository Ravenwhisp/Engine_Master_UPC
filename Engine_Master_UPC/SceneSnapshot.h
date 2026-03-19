#pragma once

#include <memory>
#include <vector>

#include "UID.h"

class Scene;
class GameObject;
class Component;
class CameraComponent;

class SceneSnapshot
{
private:
    Scene* m_scene = nullptr;

    std::vector<UID> m_originalComponentIDs;
    std::vector<Component*> m_clonedComponents;

    std::vector<const GameObject*> m_originalGameObjects;
    std::vector<GameObject*> m_clonedGameObjects;

    std::vector<std::unique_ptr<GameObject>> m_allObjects;
    std::vector<GameObject*> m_rootObjects;
    CameraComponent* m_defaultCamera = nullptr;

public:
    SceneSnapshot();
    ~SceneSnapshot();

    void init(const Scene& scene);
    void applyTo(Scene& scene);

    void registerComponent(UID id, Component* component);
    void registerGameObject(const GameObject* original, GameObject* clone);

    GameObject* getClonedGameObject(const GameObject* original) const;
    Component* getClonedComponent(UID id) const;

    template<typename T>
    T* getClonedComponentAs(UID id) const
    {
        Component* comp = getClonedComponent(id);
        return comp ? dynamic_cast<T*>(comp) : nullptr;
    }

private:
    std::unique_ptr<GameObject> cloneRecursive(GameObject* original);
    void fixReferences();
};