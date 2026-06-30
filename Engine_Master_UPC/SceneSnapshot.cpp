#include "Globals.h"
#include "SceneSnapshot.h"

#include "Scene.h"
#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "CameraComponent.h"


SceneSnapshot::SceneSnapshot() = default;
SceneSnapshot::~SceneSnapshot() = default;

void SceneSnapshot::init(const Scene& scene)
{
    for (GameObject* root : scene.getRootObjects())
    {
        auto clonedRoot = cloneRecursive(root);

        std::vector<std::unique_ptr<GameObject>> all;
        all.push_back(std::move(clonedRoot));
        for (size_t i = 0; i < all.size(); ++i)
            for (auto& child : all[i]->releaseChildren())
                all.push_back(std::move(child));

        for (auto& go : all)
        {
            GameObject* raw = go.get();
            if (raw->GetTransform()->getRoot() == nullptr)
                m_rootObjects.push_back(raw);
            m_allObjects.push_back(std::move(go));
        }
    }

    fixReferences();

    CameraComponent* oldCameraComponent = scene.getDefaultCamera();
    if (oldCameraComponent)
    {
        m_defaultCamera = (CameraComponent*)m_resolver.getClonedComponent(oldCameraComponent->getID());
    }
}

std::unique_ptr<GameObject> SceneSnapshot::cloneRecursive(GameObject* original)
{
    std::unique_ptr<GameObject> clone = original->clone();

    m_resolver.registerGameObject(original, clone.get());

    const auto& originalComponents = original->GetAllComponents();
    const auto& clonedComponents = clone->GetAllComponents();

    for (size_t i = 0; i < originalComponents.size(); ++i)
    {
        m_resolver.registerComponent(originalComponents[i]->getID(), clonedComponents[i]);
    }

    registerDescendants(original, clone.get());

    return clone;
}

void SceneSnapshot::registerDescendants(GameObject* origParent, GameObject* cloneParent)
{
    const auto& origKids = origParent->GetTransform()->getAllChildren();
    const auto& cloneKids = cloneParent->GetTransform()->getAllChildren();

    for (size_t i = 0; i < origKids.size() && i < cloneKids.size(); ++i)
    {
        m_resolver.registerGameObject(origKids[i], cloneKids[i]);

        const auto& origComps = origKids[i]->GetAllComponents();
        const auto& cloneComps = cloneKids[i]->GetAllComponents();
        for (size_t j = 0; j < origComps.size() && j < cloneComps.size(); ++j)
        {
            m_resolver.registerComponent(origComps[j]->getID(), cloneComps[j]);
        }

        registerDescendants(origKids[i], cloneKids[i]);
    }
}

void SceneSnapshot::fixReferences()
{
    for (GameObject* root : m_rootObjects)
    {
        std::vector<GameObject*> stack;
        stack.push_back(root);
        while (!stack.empty())
        {
            GameObject* obj = stack.back();
            stack.pop_back();
            for (Component* component : obj->GetAllComponents())
                component->fixReferences(m_resolver);
            for (GameObject* child : obj->GetTransform()->getAllChildren())
                stack.push_back(child);
        }
    }
}

void SceneSnapshot::applyTo(Scene& scene)
{
    scene.clearScene();

    scene.m_allObjects = std::move(m_allObjects);
    scene.m_rootObjects = std::move(m_rootObjects);
    scene.setDefaultCamera(m_defaultCamera);

    for (size_t i = 0; i < scene.m_allObjects.size(); ++i)
        scene.m_objectIndexMap[scene.m_allObjects[i].get()] = i;

    for (const auto& go : scene.m_allObjects)
    {
        go->init();
    }

    scene.markDirty();
}