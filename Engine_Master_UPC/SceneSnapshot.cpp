#include "Globals.h"
#include "SceneSnapshot.h"

#include "Scene.h"
#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "CameraComponent.h"

#include "SceneReferenceResolver.h"

SceneSnapshot::SceneSnapshot() = default;
SceneSnapshot::~SceneSnapshot() = default;

void SceneSnapshot::init(const Scene& scene)
{
    for (GameObject* root : scene.getRootObjects())
    {
        auto clonedRoot = cloneRecursive(root);

        m_rootObjects.push_back(clonedRoot.get());
        m_allObjects.push_back(std::move(clonedRoot));
    }

    fixReferences();
}

std::unique_ptr<GameObject> SceneSnapshot::cloneRecursive(GameObject* original)
{
    auto clone = original->clone();

    registerGameObject(original, clone.get());

    const auto& originalComponents = original->GetAllComponents();
    const auto& clonedComponents = clone->GetAllComponents();

    for (size_t i = 0; i < originalComponents.size(); ++i)
    {
        registerComponent(originalComponents[i]->getID(), clonedComponents[i]);
    }

    if (original->GetComponent(ComponentType::CAMERA))
    {
        m_defaultCamera = clone->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
    }

    for (GameObject* child : original->GetTransform()->getAllChildren())
    {
        auto clonedChild = cloneRecursive(child);

        clonedChild->GetTransform()->setRoot(clone->GetTransform());
        clone->GetTransform()->addChild(clonedChild.get());

        m_allObjects.push_back(std::move(clonedChild));
    }

    return clone;
}

void SceneSnapshot::fixReferences()
{
    SceneReferenceResolver resolver;

    for (size_t i = 0; i < m_originalGameObjects.size(); ++i)
    {
        resolver.registerGameObject(m_originalGameObjects[i], m_clonedGameObjects[i]);
    }

    for (size_t i = 0; i < m_originalComponentIDs.size(); ++i)
    {
        resolver.registerComponent(m_originalComponentIDs[i], m_clonedComponents[i]);
    }

    for (const auto& obj : m_allObjects)
    {
        for (Component* component : obj->GetAllComponents())
        {
            component->fixReferences(resolver);
        }
    }
}

void SceneSnapshot::applyTo(Scene& scene)
{
    scene.clearScene();

    scene.m_allObjects = std::move(m_allObjects);
    scene.m_rootObjects = std::move(m_rootObjects);
    scene.setDefaultCamera(m_defaultCamera);
}

void SceneSnapshot::registerGameObject(const GameObject* original, GameObject* clone)
{
    m_originalGameObjects.push_back(original);
    m_clonedGameObjects.push_back(clone);
}

void SceneSnapshot::registerComponent(UID id, Component* component)
{
    m_originalComponentIDs.push_back(id);
    m_clonedComponents.push_back(component);
}

GameObject* SceneSnapshot::getClonedGameObject(const GameObject* original) const
{
    for (size_t i = 0; i < m_originalGameObjects.size(); ++i)
    {
        if (m_originalGameObjects[i] == original)
            return m_clonedGameObjects[i];
    }
    return nullptr;
}

Component* SceneSnapshot::getClonedComponent(UID id) const
{
    for (size_t i = 0; i < m_originalComponentIDs.size(); ++i)
    {
        if (m_originalComponentIDs[i] == id)
            return m_clonedComponents[i];
    }
    return nullptr;
}
