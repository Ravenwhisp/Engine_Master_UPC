#include "Globals.h"

#include "Scene.h"
#include "Application.h"
#include "Settings.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"
#include "ModuleD3D12.h"
#include "ModuleScene.h"
#include "ModuleMusic.h"

#include "GameObject.h"
#include "Component.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "Quadtree.h"
#include "SceneSnapshot.h"
#include "Transform.h"
#include "SceneReferenceResolver.h"

#include "IArchive.h"

#include "TriggerSystem.h"
#include "TriggerComponent.h"

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

    gameCamera->GetTransform()->setPosition(Vector3(5.0f, 10.0f, 5.0f));
    gameCamera->GetTransform()->setRotation(Quaternion::CreateFromYawPitchRoll(-IM_PI / 4, -IM_PI / 4, 0.0f));

    gameCamera->AddComponent(ComponentType::CAMERA);
    gameCamera->SetName("Camera");
    setDefaultCamera(gameCamera->GetComponentAs<CameraComponent>(ComponentType::CAMERA));

    commitGameObject(std::move(gameCamera));

    initLoadedObjects();

    createDirectionalLightOnInit();
    markDirty();
    return true;
}

void Scene::update()
{
    releasePendingDestroyedGameObjects();
    removePendingGameObjects();

    if (app->getCurrentEngineState() == ENGINE_STATE::PLAYING)
    {
        const size_t count = m_allObjects.size();
        for (size_t i = 0; i < count; ++i)
        {
            if (m_allObjects[i]->GetActive())
            {
                m_allObjects[i]->update();
            }
        }

        for (size_t i = 0; i < count; ++i)
        {
            if (m_allObjects[i]->GetActive())
            {
                m_allObjects[i]->lateUpdate();
            }
        }

        if (m_triggerSystem)
        {
            m_triggerSystem->update();
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
GameObject* Scene::createGameObject()
{
    std::unique_ptr<GameObject> newGameObject = std::make_unique<GameObject>(GenerateUID());
    GameObject* rawPtr = newGameObject.get();
    rawPtr->init();
    rawPtr->GetTransform()->setPosition(Vector3(0.0f, 0.0f, 0.0f));

    rawPtr->onTransformChange();

    commitGameObject(std::move(newGameObject));
    markDirty();

    return rawPtr;
}

GameObject* Scene::createGameObjectWithUID(UID id, UID transformUID)
{
    auto newGameObject = std::make_unique<GameObject>(id, transformUID);
    GameObject* raw = newGameObject.get();

    raw->onTransformChange();

    commitGameObject(std::move(newGameObject));
    markDirty();

    return raw;
}

void Scene::commitGameObject(std::unique_ptr<GameObject> gameObject)
{
    GameObject* raw = gameObject.get();
    m_allObjects.push_back(std::move(gameObject));
    m_objectIndexMap[raw] = m_allObjects.size() - 1;
}

void Scene::initLoadedObjects()
{
    for (GameObject* root : getRootObjects())
    {
        if (root)
        {
            root->init();
        }
    }
}

GameObject* Scene::findGameObjectByUID(UID uuid)
{
    ensureIndicesFresh();
    auto it = m_uidIndex.find(uuid);
    return it != m_uidIndex.end() ? it->second : nullptr;
}

void Scene::removeGameObject(UID uuid)
{
    GameObject* target = findGameObjectByUID(uuid);
    if (!target)
    {
        return;
    }

    destroyGameObject(target);
}

void Scene::markGameObjectForRemoval(UID uuid)
{
    m_objectsToRemove.insert(uuid);
}

void Scene::removePendingGameObjects()
{
    for (const UID& objectUid : m_objectsToRemove)
    {
        removeGameObject(objectUid);
    }

    m_objectsToRemove.clear();
}

void Scene::releasePendingDestroyedGameObjects()
{
    CommandQueue* commandQueue = app->getModuleD3D12()->getCommandQueue();

    {
        bool needsSignal = false;
        for (const auto& p : m_pendingDestroyedObjects)
        {
            if (p.fenceValue == 0) { needsSignal = true; break; }
        }
        if (needsSignal)
        {
            const uint64_t fenceValue = commandQueue->signal();
            for (auto& p : m_pendingDestroyedObjects)
            {
                if (p.fenceValue == 0) p.fenceValue = fenceValue;
            }
        }
    }

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
    std::vector<std::unique_ptr<GameObject>> all;
    all.push_back(std::move(gameObject));

    for (size_t i = 0; i < all.size(); ++i)
    {
        for (auto& child : all[i]->releaseChildren())
        {
            all.push_back(std::move(child));
        }
    }

    std::vector<GameObject*> newGOs;
    newGOs.reserve(all.size());

    for (auto& go : all)
    {
        GameObject* raw = go.get();
        newGOs.push_back(raw);
        commitGameObject(std::move(go));
    }

    markDirty();

    fixReferencesFor(newGOs);
}

void Scene::destroyGameObject(GameObject* obj)
{
    if (!obj) return;

    ModuleEditor* editor = app->getModuleEditor();

    if (isDescendant(obj, editor->getSelectedGameObject()))
    {
        editor->setSelectedGameObject(nullptr);
    }

    auto children = obj->GetTransform()->getAllChildren();

    for (GameObject* child : children)
    {
        destroyGameObject(child);
    }

    Transform* parent = obj->GetTransform()->getRoot();

    if (parent)
    {
        parent->removeChild(obj->GetID());
    }

    auto mapIt = m_objectIndexMap.find(obj);
    if (mapIt == m_objectIndexMap.end()) return;

    const size_t idx = mapIt->second;
    const size_t lastIdx = m_allObjects.size() - 1;

    app->getModuleScene()->removeGameObjectFromQuadtree(*m_allObjects[idx].get());

    m_pendingDestroyedObjects.push_back(
        PendingDestroyedGameObject{
            std::move(m_allObjects[idx]),
            0
        });

    if (idx != lastIdx)
    {
        m_allObjects[idx] = std::move(m_allObjects[lastIdx]);
        m_objectIndexMap[m_allObjects[idx].get()] = idx;
    }

    m_allObjects.pop_back();
    m_objectIndexMap.erase(mapIt);
    markDirty();
}

bool Scene::isDescendant(GameObject* root, GameObject* candidate) const
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
        if (isDescendant(child, candidate))
        {
            return true;
        }
    }

    return false;
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

    commitGameObject(std::move(go));
    markDirty();

    return raw;
}

#pragma endregion

const std::vector<GameObject*>& Scene::getAllGameObjects() const
{
    ensureIndicesFresh();
    return m_allGameObjectsCache;
}

void Scene::ensureIndicesFresh() const
{
    if (!m_hierarchyCacheDirty)
    {
        return;
    }

    m_allGameObjectsCache.clear();
    m_allGameObjectsCache.reserve(m_allObjects.size());

    m_rootObjectsCache.clear();
    m_rootObjectsCache.reserve(m_allObjects.size());

    m_uidIndex.clear();
    m_uidIndex.reserve(m_allObjects.size());

    for (const auto& obj : m_allObjects)
    {
        m_uidIndex[obj->GetID()] = obj.get();

        if (obj->GetTransform()->getRoot() != nullptr)
            continue;

        m_rootObjectsCache.push_back(obj.get());

        m_allGameObjectsCache.push_back(obj.get());

        for (size_t j = m_allGameObjectsCache.size() - 1; j < m_allGameObjectsCache.size(); ++j)
        {
            for (GameObject* child : m_allGameObjectsCache[j]->GetTransform()->getAllChildren())
            {
                m_allGameObjectsCache.push_back(child);
            }
        }
    }

    m_hierarchyCacheDirty = false;
}

const std::vector<GameObject*>& Scene::getRootObjects() const
{
    ensureIndicesFresh();
    return m_rootObjectsCache;
}

bool Scene::containsGameObject(const GameObject* go) const
{
    if (!go)
    {
        return false;
    }

    return m_objectIndexMap.find(const_cast<GameObject*>(go)) != m_objectIndexMap.end();
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

    m_rootObjectsCache.clear();
    m_allObjects.clear();

    m_objectIndexMap.clear();
    m_uidIndex.clear();
    m_allGameObjectsCache.clear();
    m_hierarchyCacheDirty = true;
    m_defaultCamera = nullptr;
    m_objectsToRemove.clear();
    markDirty();
}

void Scene::markDirty()
{
    m_componentCacheDirty = true;
    m_hierarchyCacheDirty = true;

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
    auto it = std::remove(m_loadedBanks.begin(), m_loadedBanks.end(), bank);
    m_loadedBanks.erase(it, m_loadedBanks.end());
}

void Scene::unloadSoundBanks()
{
    app->getModuleMusic()->unloadAllBanks();
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
        {
            soundData.banks = m_loadedBanks;
        }
        archive.beginObject("SoundBanks");
        soundData.serialize(archive);
        archive.endObject();
        if (archive.mode() == ArchiveMode::Input)
        {
            m_loadedBanks = std::move(soundData.banks);
        }
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
        archive.beginArray(goCount, "GameObjects");

        struct GoMeta { uint64_t uid; uint64_t transformUid; uint64_t parentUid; };
        std::vector<GoMeta> goMeta;
        goMeta.reserve(goCount);
        std::vector<GameObject*> gos;
        gos.reserve(goCount);

        for (uint32_t i = 0; i < goCount; ++i)
        {
            archive.beginObject();

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
        archive.endArray();

        for (size_t i = 0; i < gos.size(); ++i)
        {
            if (goMeta[i].parentUid == 0) continue;

            GameObject* child = gos[i];
            GameObject* parent = findGameObjectByUID((UID)goMeta[i].parentUid);
            if (parent)
            {
                child->GetTransform()->setRoot(parent->GetTransform());
                parent->GetTransform()->addChild(child);
            }
        }

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
        archive.beginArray(goCount, "GameObjects");

        for (uint32_t i = 0; i < goCount; ++i)
        {
            GameObject* go = allGOs[i];
            archive.beginObject();

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
        archive.endArray();
    }
}

void Scene::fixReferences()
{
    fixReferencesFor(getAllGameObjects());
}

void Scene::fixReferencesFor(const std::vector<GameObject*>& gos)
{
    SceneReferenceResolver resolver;

    for (GameObject* obj : gos)
    {
        resolver.registerGameObject(obj, obj);
        for (Component* c : obj->GetAllComponents())
        {
            resolver.registerComponent(c->getID(), c);
        }
    }

    for (GameObject* obj : gos)
    {
        for (Component* c : obj->GetAllComponents())
        {
            c->fixReferences(resolver);
        }
    }
}
