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

        m_rootObjects.push_back(clonedRoot.get());
        m_allObjects.push_back(std::move(clonedRoot));
    }

    fixReferences();

    CameraComponent* oldCameraComponent = scene.getDefaultCamera();
    if (oldCameraComponent)
    {
        m_defaultCamera = (CameraComponent *)m_resolver.getClonedComponent(oldCameraComponent->getID());
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
    for (const auto& obj : m_allObjects)
    {
        for (Component* component : obj->GetAllComponents())
        {
            component->fixReferences(m_resolver);
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
