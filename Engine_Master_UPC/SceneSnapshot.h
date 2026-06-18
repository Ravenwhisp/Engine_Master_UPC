#pragma once

#include <memory>
#include <vector>

#include "UID.h"
#include "SceneReferenceResolver.h"

class Scene;
class GameObject;
class Component;
class CameraComponent;


class SceneSnapshot
{
private:
    SceneReferenceResolver m_resolver;

    std::vector<std::unique_ptr<GameObject>> m_allObjects;
    CameraComponent* m_defaultCamera = nullptr;

public:
    SceneSnapshot();
    ~SceneSnapshot();

    void init(const Scene& scene);
    void applyTo(Scene& scene);

private:
    std::unique_ptr<GameObject> cloneRecursive(GameObject* original);
    void registerDescendants(GameObject* origParent, GameObject* cloneParent);
    void fixReferences();
};