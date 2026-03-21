#include "Globals.h"

#include "Scene.h"
#include "Application.h"
#include "Settings.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"

#include "GameObject.h"
#include "Component.h"
#include "MeshRenderer.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "Quadtree.h"
#include "SceneSnapshot.h"
#include "Transform.h"

#include <limits>
#include <algorithm>

Scene::Scene() = default;
Scene::~Scene() = default;

#pragma region GameLoop

bool Scene::init()
{
    m_lighting.ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting.ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    auto gameCamera = std::make_unique<GameObject>(GenerateUID());
    GameObject* rawPtr = gameCamera.get();

    gameCamera->GetTransform()->setPosition(Vector3(-5.0f, 10.0f, -5.0f));
    gameCamera->GetTransform()->setRotation(Quaternion::CreateFromYawPitchRoll(IM_PI / 4, IM_PI / 4, 0.0f));

    gameCamera->AddComponent(ComponentType::CAMERA);
    gameCamera->SetName("Camera");

    m_allObjects.push_back(std::move(gameCamera));
    m_rootObjects.push_back(rawPtr);

    for (const auto& go : m_allObjects)
    {
        go->init();
    }

    createDirectionalLightOnInit();
    applySkyBoxToRenderer();

    return true;
}

void Scene::update()
{
    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        for (const auto& go : m_allObjects)
        {
            if (go->GetActive())
            {
                go->update();
            }
        }
    }
}

bool Scene::cleanUp()
{
    clearScene();
    return true;
}

#pragma endregion


#pragma region CRUD
void Scene::createGameObject()
{
    std::unique_ptr<GameObject> newGameObject = std::make_unique<GameObject>(GenerateUID());
    GameObject* rawPtr = newGameObject.get();
    rawPtr->init();
    rawPtr->GetTransform()->setPosition(Vector3(1.0f, 0.0f, 1.0f));

    m_allObjects.push_back(std::move(newGameObject));
    m_rootObjects.push_back(rawPtr);

    rawPtr->onTransformChange();
}

GameObject* Scene::createGameObjectWithUID(UID id, UID transformUID)
{
    auto newGameObject = std::make_unique<GameObject>(id, transformUID);
    GameObject* raw = newGameObject.get();

    raw->init();

    m_allObjects.push_back(std::move(newGameObject));
    m_rootObjects.push_back(raw);

    raw->onTransformChange();

    return raw;
}

GameObject* Scene::findGameObjectByUID(UID uuid)
{
    for (const auto& root : m_allObjects)
    {
        if (root->GetID() == uuid)
        {
            return root.get();
        }

        if (GameObject* found = findInWindowHierarchy(root.get(), uuid))
        {
            return found;
        }
    }
    return nullptr;
}

void Scene::removeGameObject(UID uuid)
{
    GameObject* target = nullptr;

    for (const auto& root : m_allObjects)
    {
        if (root->GetID() == uuid)
        {
            target = root.get();
            break;
        }

        target = findInWindowHierarchy(root.get(), uuid);
        if (target)
        {
            break;
        }
    }

    if (!target)
    {
        return;
    }

    destroyWindowHierarchy(target);
}

void Scene::addGameObject(std::unique_ptr<GameObject> gameObject)
{
    m_allObjects.push_back(std::move(gameObject));
}

void Scene::destroyGameObject(GameObject* gameObject)
{
    removeFromRootList(gameObject);

    auto it = std::find_if(
        m_allObjects.begin(),
        m_allObjects.end(),
        [gameObject](const std::unique_ptr<GameObject>& ptr)
        {
            return ptr.get() == gameObject;
        });

    if (it != m_allObjects.end())
    {
        (*it)->cleanUp();
        m_allObjects.erase(it);
    }
}

GameObject* Scene::findInWindowHierarchy(GameObject* current, UID uuid)
{
    for (GameObject* child : current->GetTransform()->getAllChildren())
    {
        if (child->GetID() == uuid)
        {
            return child;
        }

        GameObject* found = findInWindowHierarchy(child, uuid);
        if (found)
        {
            return found;
        }
    }

    return nullptr;
}

void Scene::destroyWindowHierarchy(GameObject* obj)
{
    auto children = obj->GetTransform()->getAllChildren();

    for (GameObject* child : children)
    {
        destroyWindowHierarchy(child);
    }

    Transform* parent = obj->GetTransform()->getRoot();

    if (parent)
    {
        parent->removeChild(obj->GetID());
    }

    destroyGameObject(obj);
}

GameObject* Scene::createDirectionalLightOnInit()
{
    auto go = std::make_unique<GameObject>(GenerateUID());
    GameObject* raw = go.get();

    auto component = raw->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
    raw->RemoveComponent(component);

    raw->SetName("Directional Light");
    raw->AddComponent(ComponentType::LIGHT);

    auto* light = raw->GetComponentAs<LightComponent>(ComponentType::LIGHT);
    if (light)
    {
        light->setTypeDirectional();
        light->editData().common.color = Vector3::One;
        light->editData().common.intensity = 1.0f;
        light->setActive(true);
        light->sanitize();
    }

    raw->GetTransform()->setRotationEuler({ 180.f, 0.f, 0.f });
    raw->init();

    m_allObjects.push_back(std::move(go));
    m_rootObjects.push_back(raw);

    return raw;
}

bool Scene::applySkyBoxToRenderer()
{
    return app->getModuleRender()->applySkyBoxSettings(m_skybox);
}

#pragma endregion

const std::vector<GameObject*> Scene::getAllGameObjects() const
{
    std::vector<GameObject*> result;
    result.reserve(m_allObjects.size());

    for (const auto& obj : m_allObjects)
    {
        result.push_back(obj.get());
    }

    return result;
}

void Scene::removeFromRootList(GameObject* obj)
{
    auto it = std::remove(
        m_rootObjects.begin(),
        m_rootObjects.end(),
        obj);

    m_rootObjects.erase(it, m_rootObjects.end());
}

void Scene::addToRootList(GameObject* gameObject)
{
    if (!gameObject) return;

    if (std::find(m_rootObjects.begin(), m_rootObjects.end(), gameObject) == m_rootObjects.end())
    {
        m_rootObjects.push_back(gameObject);
    }
}

const std::vector<GameObject*>& Scene::getRootObjects() const
{
    return m_rootObjects;
}

void Scene::clearScene()
{
    app->getModuleEditor()->setSelectedGameObject(nullptr);

    for (auto& go : m_allObjects)
    {
        go->cleanUp();
    }

    m_rootObjects.clear();
    m_allObjects.clear();

    m_defaultCamera = nullptr;
}
