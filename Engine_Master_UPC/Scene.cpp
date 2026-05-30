#include "Globals.h"

#include "Scene.h"
#include "Application.h"
#include "Settings.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"
#include "ModuleD3D12.h"
#include "ModuleScene.h"

#include "GameObject.h"
#include "Component.h"
#include "MeshRenderer.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "Quadtree.h"
#include "SceneSnapshot.h"
#include "Transform.h"

#include "IArchive.h"

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
    GameObject* raw = gameObject.get();
    m_allObjects.push_back(std::move(gameObject));
    m_rootObjects.push_back(raw);
    markDirty();

    SceneReferenceResolver resolver;
    std::vector<GameObject*> gos = {raw};
    for (size_t i = 0; i < gos.size(); ++i)
    {
        resolver.registerGameObject(gos[i], gos[i]);
        for (Component* c : gos[i]->GetAllComponents())
            resolver.registerComponent(c->getID(), c);

        for (GameObject* child : gos[i]->GetTransform()->getAllChildren())
            gos.push_back(child);
    }

    for (GameObject* go : gos)
        for (Component* c : go->GetAllComponents())
            c->fixReferences(resolver);
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

		app->getModuleScene()->removeGameObjectFromQuadtree(*it->get());

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
        if (obj->GetTransform()->getRoot() != nullptr)
            continue;

        result.push_back(obj.get());

        for (size_t j = result.size() - 1; j < result.size(); ++j)
        {
            for (GameObject* child : result[j]->GetTransform()->getAllChildren())
                result.push_back(child);
        }
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

#pragma region MusicBanks
const std::vector<std::string>& Scene::getLoadedBanks() const
{
    return m_loadedBanks;
}

void Scene::addLoadedBank(const std::string& bank)
{
    if (std::find(m_loadedBanks.begin(), m_loadedBanks.end(), bank) != m_loadedBanks.end())
    {
        return;
    }

    m_loadedBanks.push_back(bank);
}

void Scene::removeLoadedBank(const std::string& bank)
{
    for (auto it = m_loadedBanks.begin(); it != m_loadedBanks.end(); ++it)
    {
        if (*it != bank)
        {
            continue;
        }

        m_loadedBanks.erase(it);
        return;
    }
}
#pragma endregion

void Scene::serialize(IArchive& archive)
{
    archive.serialize(m_name, "name");

    archive.beginObject("Lighting");
    m_lighting.serialize(archive);
    archive.endObject();

    archive.beginObject("SkyBox");
    m_skybox.serialize(archive);
    archive.endObject();

    {
        SoundBanksData soundData;
        if (archive.mode() == ArchiveMode::Output)
            soundData.banks = m_loadedBanks;
        archive.beginObject("SoundBanks");
        soundData.serialize(archive);
        archive.endObject();
        if (archive.mode() == ArchiveMode::Input)
            m_loadedBanks = std::move(soundData.banks);
    }

    uint64_t defaultCameraUid = 0;
    if (archive.mode() == ArchiveMode::Output && m_defaultCamera)
    {
        GameObject* owner = m_defaultCamera->getOwner();
        defaultCameraUid = (uint64_t)owner->GetID();
    }
    archive.serialize(defaultCameraUid, "defaultCameraUid");

    if (archive.mode() == ArchiveMode::Input)
    {
        uint32_t goCount = 0;
        archive.serialize(goCount, "goCount");

        struct GoMeta { uint64_t uid; uint64_t transformUid; uint64_t parentUid; };
        std::vector<GoMeta> goMeta;
        goMeta.reserve(goCount);
        std::vector<GameObject*> gos;
        gos.reserve(goCount);

        for (uint32_t i = 0; i < goCount; ++i)
        {
            std::string key = "GameObject_" + std::to_string(i);
            archive.beginObject(key.c_str());

            uint64_t uid = 0, transformUid = 0, parentUid = 0;
            archive.serialize(uid, "uid");
            archive.serialize(transformUid, "transformUid");
            archive.serialize(parentUid, "parentUid");

            GameObject* go = createGameObjectWithUID((UID)uid, (UID)transformUid);
            go->serialize(archive);

            goMeta.push_back({uid, transformUid, parentUid});
            gos.push_back(go);

            archive.endObject();
        }

        for (size_t i = 0; i < gos.size(); ++i)
        {
            if (goMeta[i].parentUid == 0) continue;

            GameObject* child = gos[i];
            GameObject* parent = findGameObjectByUID((UID)goMeta[i].parentUid);
            if (parent)
            {
                child->GetTransform()->setRoot(parent->GetTransform());
                parent->GetTransform()->addChild(child);
                removeFromRootList(child);
            }
        }

        FixReferences();

        if (defaultCameraUid != 0)
        {
            GameObject* go = findGameObjectByUID((UID)defaultCameraUid);
            if (go)
            {
                auto* cam = go->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
                setDefaultCamera(cam);
            }
        }
    }
    else
    {
        auto allGOs = getAllGameObjects();
        uint32_t goCount = static_cast<uint32_t>(allGOs.size());
        archive.serialize(goCount, "goCount");

        for (uint32_t i = 0; i < goCount; ++i)
        {
            GameObject* go = allGOs[i];
            std::string key = "GameObject_" + std::to_string(i);
            archive.beginObject(key.c_str());

            uint64_t uid = go->GetID();
            uint64_t transformUid = go->GetTransform()->getID();
            archive.serialize(uid, "uid");
            archive.serialize(transformUid, "transformUid");

            Transform* parentTransform = go->GetTransform()->getRoot();
            uint64_t parentUid = parentTransform ? (uint64_t)parentTransform->getOwner()->GetID() : 0;
            archive.serialize(parentUid, "parentUid");

            go->serialize(archive);

            archive.endObject();
        }
    }
}

void Scene::FixReferences()
{
    SceneReferenceResolver resolver;

    for (GameObject* obj : getAllGameObjects())
    {
        resolver.registerGameObject(obj, obj);
        for (Component* c : obj->GetAllComponents())
            resolver.registerComponent(c->getID(), c);
    }

    for (GameObject* obj : getAllGameObjects())
    {
        for (Component* c : obj->GetAllComponents())
            c->fixReferences(resolver);
    }
}
