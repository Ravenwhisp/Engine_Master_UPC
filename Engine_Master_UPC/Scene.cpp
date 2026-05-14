#include "Globals.h"

#include "Scene.h"
#include "Application.h"
#include "Settings.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"
#include "ModuleD3D12.h"

#include "GameObject.h"
#include "Component.h"
#include "MeshRenderer.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "Quadtree.h"
#include "SceneSnapshot.h"
#include "Transform.h"

#include "TriggerSystem.h"
#include "TriggerComponent.h"

#include <limits>
#include <algorithm>


Scene::Scene(AssetReference& id): Asset(id, AssetType::SCENE) 
{
    m_triggerSystem = std::make_unique<TriggerSystem>();
}

Scene::~Scene() = default;

#pragma region GameLoop

bool Scene::init()
{
    m_lighting.ambientColor = LightDefaults::DEFAULT_AMBIENT_COLOR;
    m_lighting.ambientIntensity = LightDefaults::DEFAULT_AMBIENT_INTENSITY;

    auto gameCamera = std::make_unique<GameObject>(GenerateUID());
    GameObject* rawPtr = gameCamera.get();

    gameCamera->GetTransform()->setPosition(Vector3(5.0f, 10.0f, 5.0f));
    gameCamera->GetTransform()->setRotation(Quaternion::CreateFromYawPitchRoll(-IM_PI / 4, -IM_PI / 4, 0.0f));

    gameCamera->AddComponent(ComponentType::CAMERA);
    gameCamera->SetName("Camera");
    setDefaultCamera(gameCamera->GetComponentAs<CameraComponent>(ComponentType::CAMERA));

    m_allObjects.push_back(std::move(gameCamera));
    m_rootObjects.push_back(rawPtr);

    for (const auto& go : m_allObjects)
    {
        go->init();
    }

    createDirectionalLightOnInit();
    markDirty();
    return true;
}

void Scene::update()
{
    releasePendingDestroyedGameObjects();

    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        removePendingGameObjects();

        m_isUpdating = true;

        for (const auto& go : m_allObjects)
        {
            if (go->GetActive())
            {
                go->update();
            }
        }

        for (const auto& go : m_allObjects)
        {
            if (go->GetActive())
            {
                go->lateUpdate();
            }
        }

        if (m_triggerSystem)
        {
            m_triggerSystem->update();
        }

        m_isUpdating = false;

        flushPendingGameObjects();
    }
}

bool Scene::cleanUp()
{
    clearScene();
    return true;
}

#pragma endregion


#pragma region CRUD
GameObject* Scene::createGameObject()
{
    std::unique_ptr<GameObject> newGameObject = std::make_unique<GameObject>(GenerateUID());
    GameObject* rawPtr = newGameObject.get();
    rawPtr->init();
    rawPtr->GetTransform()->setPosition(Vector3(0.0f, 0.0f, 0.0f));

    rawPtr->onTransformChange();

    if (m_isUpdating)
    {
        m_pendingObjectsToAdd.push_back(std::move(newGameObject));
        m_pendingRootObjectsToAdd.push_back(rawPtr);
    }
    else
    {
        m_allObjects.push_back(std::move(newGameObject));
        m_rootObjects.push_back(rawPtr);
        markDirty();
    }

    return rawPtr;
}

GameObject* Scene::createGameObjectWithUID(UID id, UID transformUID)
{
    auto newGameObject = std::make_unique<GameObject>(id, transformUID);
    GameObject* raw = newGameObject.get();

    raw->onTransformChange();

    if (m_isUpdating)
    {
        m_pendingObjectsToAdd.push_back(std::move(newGameObject));
        m_pendingRootObjectsToAdd.push_back(raw);
    }
    else
    {
        m_allObjects.push_back(std::move(newGameObject));
        m_rootObjects.push_back(raw);
        markDirty();
    }

    return raw;
}

void Scene::initLoadedObjects()
{
    for (GameObject* root : m_rootObjects)
    {
        if (root)
        {
            root->init();
        }
    }
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

void Scene::markGameObjectForRemoval(UID uuid)
{
    if (!findGameObjectByUID(uuid)) return;

    for (const UID& objectUid : m_objectsToRemove)
    {
        if (objectUid == uuid) return;
    }

    m_objectsToRemove.push_back(uuid);
}

void Scene::removePendingGameObjects()
{
    for (const UID& objectUid : m_objectsToRemove)
    {
        removeGameObject(objectUid);
    }

    m_objectsToRemove.clear();
}

void Scene::flushPendingGameObjects()
{
    if (m_pendingObjectsToAdd.empty())
    {
        return;
    }

    for (GameObject* rootObject : m_pendingRootObjectsToAdd)
    {
        if (rootObject)
        {
            m_rootObjects.push_back(rootObject);
        }
    }

    for (auto& pendingObject : m_pendingObjectsToAdd)
    {
        m_allObjects.push_back(std::move(pendingObject));
    }

    m_pendingObjectsToAdd.clear();
    m_pendingRootObjectsToAdd.clear();

    markDirty();
}

void Scene::releasePendingDestroyedGameObjects()
{
    CommandQueue* commandQueue = app->getModuleD3D12()->getCommandQueue();

    auto it = m_pendingDestroyedObjects.begin();

    while (it != m_pendingDestroyedObjects.end())
    {
        if (commandQueue->isFenceComplete(it->fenceValue))
        {
            it->gameObject->cleanUp();
            it = m_pendingDestroyedObjects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Scene::addGameObject(std::unique_ptr<GameObject> gameObject)
{
    m_allObjects.push_back(std::move(gameObject));
    markDirty();
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
        const uint64_t fenceValue = app->getModuleD3D12()->getCommandQueue()->signal();

        m_pendingDestroyedObjects.push_back(
            PendingDestroyedGameObject{
                std::move(*it),
                fenceValue
            });
        m_allObjects.erase(it);
    }
    markDirty();
}

bool Scene::isInHierarchy(GameObject* root, GameObject* candidate) const
{
    if (!root || !candidate)
    {
        return false;
    }

    if (root == candidate)
    {
        return true;
    }

    for (GameObject* child : root->GetTransform()->getAllChildren())
    {
        if (isInHierarchy(child, candidate))
        {
            return true;
        }
    }

    return false;
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
    if (!obj)
    {
        return;
    }

    ModuleEditor* editor = app->getModuleEditor();

    if (isInHierarchy(obj, editor->getSelectedGameObject()))
    {
        editor->setSelectedGameObject(nullptr);
    }

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
    markDirty();

    return raw;
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

    auto pendingIt = std::remove(
        m_pendingRootObjectsToAdd.begin(),
        m_pendingRootObjectsToAdd.end(),
        obj);

    m_pendingRootObjectsToAdd.erase(pendingIt, m_pendingRootObjectsToAdd.end());
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

bool Scene::containsGameObject(const GameObject* go) const
{
    if (!go)
    {
        return false;
    }

    for (const auto& obj : m_allObjects)
    {
        if (obj.get() == go)
        {
            return true;
        }
    }

    return false;
}

void Scene::clearScene()
{
    app->getModuleEditor()->setSelectedGameObject(nullptr);

    clearTriggers();

    for (auto& pending : m_pendingDestroyedObjects)
    {
        if (pending.gameObject)
        {
            pending.gameObject->cleanUp();
        }
    }

    m_pendingDestroyedObjects.clear();

    for (auto& go : m_allObjects)
    {
        go->cleanUp();
    }

    m_rootObjects.clear();
    m_allObjects.clear();

    m_defaultCamera = nullptr;
    markDirty();
}

void Scene::markDirty()
{
    m_componentCacheDirty = true;

    if (app && app->getModuleRender())
    {
        app->getModuleRender()->markDebugDrawCacheDirty();
    }
}

void Scene::registerTrigger(TriggerComponent* trigger)
{
    if (m_triggerSystem)
    {
        m_triggerSystem->registerTrigger(trigger);
    }
}

void Scene::unregisterTrigger(TriggerComponent* trigger)
{
    if (m_triggerSystem)
    {
        m_triggerSystem->unregisterTrigger(trigger);
    }
}

void Scene::clearTriggers()
{
    if (m_triggerSystem)
    {
        m_triggerSystem->clear();
    }
}
