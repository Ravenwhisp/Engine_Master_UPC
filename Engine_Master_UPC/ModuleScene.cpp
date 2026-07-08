#include "Globals.h"
#include "ModuleScene.h"

#include "Application.h"
#include "Settings.h"
#include "ModuleNavigation.h"
#include "ModuleEditor.h"
#include "ModuleAssets.h"
#include "ModuleMusic.h"

#include "Scene.h"
#include "Quadtree.h"
#include "JsonArchive.h"
#include "SceneSnapshot.h"

#include "GameObject.h"
#include "MeshRenderer.h"
#include "LightComponent.h"
#include "ScriptComponent.h"
#include "ParticleSystemComponent.h"
#include "TrailComponent.h"

#include "ScenePicking.h"

ModuleScene::ModuleScene()
{
    AssetReference defaultSceneRef;
    m_scene = std::make_unique<Scene>(defaultSceneRef);
    m_staticQuadtree = std::make_unique<Quadtree>();
    m_dynamicQuadtree = std::make_unique<Quadtree>();
}

ModuleScene::~ModuleScene() = default;


void ModuleScene::requestSceneChange(const std::string& sceneName)
{
    m_pendingSceneLoad = sceneName;
}

void ModuleScene::requestSceneChange(std::shared_ptr<Scene> scene)
{
    m_pendingScene = std::move(scene);
}

#pragma region GameLoop
bool ModuleScene::init()
{
    m_scene->init();
    m_staticQuadtree->init(m_scene.get(), dd::colors::Red, dd::colors::Green);
    m_dynamicQuadtree->init(m_scene.get(), dd::colors::Cyan, dd::colors::Yellow);

    return true;
}

void ModuleScene::update()
{
    if (!m_pendingSceneLoad.empty())
    {
        loadScene(m_pendingSceneLoad);
        m_pendingSceneLoad.clear();
    }

    if (m_pendingScene)
    {
        loadScene(m_pendingScene);
        m_pendingScene.reset();
    }

    m_scene->update();

    syncQuadtreeWithSettings();
    m_staticQuadtree->update();
    m_dynamicQuadtree->update();
}

bool ModuleScene::cleanUp()
{
    clearComponentCaches();

    m_scene.reset();
    m_staticQuadtree.reset();
    m_dynamicQuadtree.reset();
    return true;
}
#pragma endregion

#pragma region Caches
void ModuleScene::clearComponentCaches()
{
    m_meshRenderers.clear();
    m_lightComponents.clear();
    m_scriptComponents.clear();
}

void ModuleScene::rebuildComponentCaches()
{
    m_meshRenderers.clear();
    m_lightComponents.clear();
    m_scriptComponents.clear();
    m_particleSystemComponents.clear();
    m_trailComponents.clear();

    for (GameObject* go : m_scene->getAllGameObjects())
    {
        if (!go->IsActiveInWindowHierarchy())
        {
            continue;
        }

        if (auto* mesh = go->GetComponentAs<MeshRenderer>(ComponentType::MODEL))
        {
            m_meshRenderers.push_back(mesh);
        }


        if (auto* light = go->GetComponentAs<LightComponent>(ComponentType::LIGHT))
        {
            m_lightComponents.push_back(light);
        }

        if (auto* script = go->GetComponentAs<ScriptComponent>(ComponentType::SCRIPT))
        {
            m_scriptComponents.push_back(script);
        }

        if (auto* particleSystem = go->GetComponentAs<ParticleSystemComponent>(ComponentType::PARTICLE_SYSTEM))
        {
            m_particleSystemComponents.push_back(particleSystem);
        }

        if (auto* particleSystem = go->GetComponentAs<TrailComponent>(ComponentType::TRAIL))
        {
            m_trailComponents.push_back(particleSystem);
        }
    }

    m_scene->clearDirty();

}

const std::vector<MeshRenderer*>& ModuleScene::getMeshRenderers()
{
    if (m_scene->isComponentCacheDirty())
    {
        rebuildComponentCaches();
    }
    return m_meshRenderers;
}

const std::vector<MeshRenderer*> ModuleScene::getDeferredMeshRenderers()
{
    if (m_scene->isComponentCacheDirty())
    {
        rebuildComponentCaches();
    }

    std::vector<MeshRenderer*> meshRenderers = {};
    for (MeshRenderer* renderer : m_meshRenderers)
    {
        if (renderer->getRenderMode() == RenderMode::DEFAULT)
        {
            meshRenderers.push_back(renderer);
        }
    }

    return meshRenderers;
}

const std::vector<MeshRenderer*> ModuleScene::getForwardMeshRenderers()
{
    if (m_scene->isComponentCacheDirty())
    {
        rebuildComponentCaches();
    }

    std::vector<MeshRenderer*> meshRenderers = {};
    for (MeshRenderer* renderer : m_meshRenderers)
    {
        if (renderer->getRenderMode() != RenderMode::DEFAULT)
        {
            meshRenderers.push_back(renderer);
        }
    }

    return meshRenderers;
}

const std::vector<MeshRenderer*> ModuleScene::getForwardMeshRenderers(RenderMode mode)
{
    if (m_scene->isComponentCacheDirty())
    {
        rebuildComponentCaches();
    }

    std::vector<MeshRenderer*> meshRenderers = {};
    for (MeshRenderer* renderer : m_meshRenderers)
    {
        if (renderer->getRenderMode() == mode)
        {
            meshRenderers.push_back(renderer);
        }
    }

    return meshRenderers;
}

const std::vector<MeshRenderer*> ModuleScene::getVisibleMeshRenderers()
{
    if (app->getSettings()->frustumCulling.enabled)
    {
        m_visibleMeshRenderers.clear();
        for (GameObject* gO : m_staticQuadtree->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer)
            {
                m_visibleMeshRenderers.push_back(renderer);
            }
        }

        for (GameObject* gO : m_dynamicQuadtree->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer)
            {
                m_visibleMeshRenderers.push_back(renderer);
            }
        }
        return m_visibleMeshRenderers;
    }
    return getMeshRenderers();
}

const std::vector<MeshRenderer*> ModuleScene::getVisibleDeferredMeshRenderers()
{
    if (app->getSettings()->frustumCulling.enabled)
    {
        std::vector<MeshRenderer*> visibleMeshRenderers = {};
        for (GameObject* gO : m_staticQuadtree->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer && renderer->getRenderMode() == RenderMode::DEFAULT)
            {
                visibleMeshRenderers.push_back(renderer);
            }
        }

        for (GameObject* gO : m_dynamicQuadtree->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer && renderer->getRenderMode() == RenderMode::DEFAULT)
            {
                visibleMeshRenderers.push_back(renderer);
            }
        }
        return visibleMeshRenderers;
    }
    return app->getModuleScene()->getDeferredMeshRenderers();
}

const std::vector<MeshRenderer*> ModuleScene::getVisibleForwardMeshRenderers()
{
    if (app->getSettings()->frustumCulling.enabled)
    {
        std::vector<MeshRenderer*> visibleMeshRenderers = {};
        for (GameObject* gO : m_staticQuadtree->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer && renderer->getRenderMode() != RenderMode::DEFAULT)
            {
                visibleMeshRenderers.push_back(renderer);
            }
        }

        for (GameObject* gO : m_dynamicQuadtree->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer && renderer->getRenderMode() != RenderMode::DEFAULT)
            {
                visibleMeshRenderers.push_back(renderer);
            }
        }
        return visibleMeshRenderers;
    }
    return app->getModuleScene()->getForwardMeshRenderers();
}

const std::vector<MeshRenderer*> ModuleScene::getVisibleForwardMeshRenderers(RenderMode mode)
{
    if (app->getSettings()->frustumCulling.enabled)
    {
        std::vector<MeshRenderer*> visibleMeshRenderers = {};
        for (GameObject* gO : m_staticQuadtree->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer && renderer->getRenderMode() == mode)
            {
                visibleMeshRenderers.push_back(renderer);
            }
        }

        for (GameObject* gO : m_dynamicQuadtree->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer && renderer->getRenderMode() == mode)
            {
                visibleMeshRenderers.push_back(renderer);
            }
        }
        return visibleMeshRenderers;
    }
    return app->getModuleScene()->getForwardMeshRenderers(mode);
}


const std::vector<LightComponent*>& ModuleScene::getLightComponents()
{
    if (m_scene->isComponentCacheDirty())
    {
        rebuildComponentCaches();
    }
    return m_lightComponents;
}

const std::vector<ScriptComponent*>& ModuleScene::getScriptComponents()
{
    if (m_scene->isComponentCacheDirty())
    {
        rebuildComponentCaches();
    }

    return m_scriptComponents;
}
const std::vector<ParticleSystemComponent*>& ModuleScene::getParticleSystemComponents()
{
    if (m_scene->isComponentCacheDirty())
    {
        rebuildComponentCaches();
    }

    return m_particleSystemComponents;
}
const std::vector<TrailComponent*>& ModuleScene::getTrailComponents()
{
    if (m_scene->isComponentCacheDirty())
    {
        rebuildComponentCaches();
    }

    return m_trailComponents;
}

#pragma endregion

#pragma region Persistence
void ModuleScene::saveScene()
{
    app->getModuleAssets()->save(*m_scene.get());
}

bool ModuleScene::loadScene(const std::string& sceneName)
{
    clearRuntimeSceneSystems();
    clearComponentCaches();
    m_scene->unloadSoundBanks();

    std::string path = "Assets/Scenes/" + sceneName + ".scene";

    JsonArchive archive(ArchiveMode::Input);
    if (!archive.loadFile(path))
    {
        DEBUG_ERROR("[ModuleScene] Failed to load scene file: %s", path.c_str());
        return false;
    }

    AssetReference ref(GenerateUID());
    auto newScene = std::make_unique<Scene>(ref);
    newScene->serialize(archive);
    newScene->setName(sceneName.c_str());
    newScene->FixReferences();
    newScene->initLoadedObjects();

    m_scene = std::move(newScene);
    m_scene->markDirty();

    m_staticQuadtree = std::make_unique<Quadtree>();
    m_staticQuadtree->init(m_scene.get(), dd::colors::Red, dd::colors::Green);
    m_dynamicQuadtree = std::make_unique<Quadtree>();
    m_dynamicQuadtree->init(m_scene.get(), dd::colors::Cyan, dd::colors::Yellow);

    if (app->getModuleNavigation()->loadNavMeshForScene(sceneName.c_str()))
    {
        DEBUG_LOG("[ModuleScene] NavMesh loaded: %s", sceneName.c_str());
    }
    else
    {
        DEBUG_WARN("[ModuleScene] NavMesh not found for scene: %s", sceneName.c_str());
    }

    app->getModuleEditor()->setSelectedGameObject(nullptr);

#ifdef GAME_RELEASE
    app->getSettings()->frustumCulling.enabled = true;
#endif

    rebuildComponentCaches();

    for (const auto& ref : m_scene->getLoadedBankRefs())
    {
        app->getModuleMusic()->loadBank(ref);
    }

    m_scene->resolveLoadedBankNames();

    initializeRuntimeSceneSystems();

    return true;
}

bool ModuleScene::loadScene(std::shared_ptr<Scene> scene)
{
    clearRuntimeSceneSystems();
    clearComponentCaches();

    auto sceneName = scene->getName();

    if (!sceneName)
    {
        DEBUG_ERROR("[ModuleScene] Failed to load scene: %s", sceneName);
        return false;
    }

    m_scene = std::move(scene);
    m_scene->setName(sceneName);
    m_scene->markDirty();

    m_staticQuadtree = std::make_unique<Quadtree>();
    m_staticQuadtree->init(m_scene.get(), dd::colors::Red, dd::colors::Green);
    m_dynamicQuadtree = std::make_unique<Quadtree>();
    m_dynamicQuadtree->init(m_scene.get(), dd::colors::Cyan, dd::colors::Yellow);

    if (app->getModuleNavigation()->loadNavMeshForScene(sceneName))
    {
        DEBUG_LOG("[ModuleScene] NavMesh loaded: %s", sceneName);
    }
    else
    {
        DEBUG_WARN("[ModuleScene] NavMesh not found for scene: %s", sceneName);
    }

    app->getModuleEditor()->setSelectedGameObject(nullptr);

#ifdef GAME_RELEASE
    app->getSettings()->frustumCulling.enabled = true;
#endif

    rebuildComponentCaches();

    for (const auto& ref : m_scene->getLoadedBankRefs())
    {
        app->getModuleMusic()->loadBank(ref);
    }

    m_scene->resolveLoadedBankNames();

    return true;
}

bool ModuleScene::loadScene(const AssetReference& ref)
{
    AssetReference mutableRef = ref;
    auto scene = app->getModuleAssets()->load<Scene>(mutableRef);
    if (!scene)
    {
        DEBUG_ERROR("[ModuleScene] Failed to load scene from Library.");
        return false;
    }

    scene->FixReferences();
    scene->initLoadedObjects();

    return loadScene(scene);
}

#pragma endregion

#pragma region Snapshot
SceneSnapshot* ModuleScene::takeSnapshot() const
{
    SceneSnapshot* sceneSnapshot = new SceneSnapshot();
    sceneSnapshot->init(*m_scene.get());

    return sceneSnapshot;
}

void ModuleScene::loadFromSnapshot(SceneSnapshot& snapshot)
{
    clearComponentCaches();

    snapshot.applyTo(*m_scene.get());
    m_scene->markDirty();

    m_staticQuadtree = std::make_unique<Quadtree>();
    m_staticQuadtree->init(m_scene.get(), dd::colors::Red, dd::colors::Green);
    m_dynamicQuadtree = std::make_unique<Quadtree>();
    m_dynamicQuadtree->init(m_scene.get(), dd::colors::Cyan, dd::colors::Yellow);

    rebuildComponentCaches();
}
#pragma endregion

#pragma region Quadtree
void ModuleScene::syncQuadtreeWithSettings()
{
    if (!m_staticQuadtree || !m_dynamicQuadtree)
    {
        return;
    }

    const bool shouldUseQuadtrees =
        app->getSettings()->frustumCulling.enabled;

    if (shouldUseQuadtrees)
    {
        if (!m_staticQuadtree->getIsBuilded())
        {
            m_staticQuadtree->build(m_staticLayers);
        }

        if (!m_dynamicQuadtree->getIsBuilded())
        {
            m_dynamicQuadtree->build(m_dynamicLayers);
        }
    }
    else
    {
        if (m_staticQuadtree->getIsBuilded())
        {
            m_staticQuadtree->clear();
        }

        if (m_dynamicQuadtree->getIsBuilded())
        {
            m_dynamicQuadtree->clear();
        }
    }
}

void ModuleScene::moveGameObjectInQuadtrees(GameObject& gameObject)
{
    const Layer layer = gameObject.GetLayer();

    if (std::find(m_dynamicLayers.begin(), m_dynamicLayers.end(), layer) != m_dynamicLayers.end())
    {
        m_dynamicQuadtree->move(gameObject);
    }
    else if (std::find(m_staticLayers.begin(), m_staticLayers.end(), layer) != m_staticLayers.end())
    {
        m_staticQuadtree->move(gameObject);
    }
}

void ModuleScene::removeGameObjectFromQuadtree(GameObject& gameObject)
{
    const Layer layer = gameObject.GetLayer();

    if (std::find(m_dynamicLayers.begin(), m_dynamicLayers.end(), layer) != m_dynamicLayers.end())
    {
        m_dynamicQuadtree->remove(gameObject);
    }
    else if (std::find(m_staticLayers.begin(), m_staticLayers.end(), layer) != m_staticLayers.end())
    {
        m_staticQuadtree->remove(gameObject);
    }
}
#pragma endregion

#pragma region ObjectPicking
std::vector<GameObjectPickHit> ModuleScene::collectAABBHits(const Ray& worldRay)
{
    std::vector<GameObjectPickHit> hits;

    const std::vector<MeshRenderer*>& meshRenderers = getMeshRenderers();

    for (MeshRenderer* meshRenderer : meshRenderers)
    {
        if (!meshRenderer)
        {
            continue;
        }

        GameObject* owner = meshRenderer->getOwner();

        if (!owner || !owner->IsActiveInWindowHierarchy())
        {
            continue;
        }

        float distance = FLT_MAX;

        if (!ScenePicking::intersectMeshRendererAABB(meshRenderer, worldRay, distance))
        {
            continue;
        }

        GameObjectPickHit hit;
        hit.gameObject = owner;
        hit.meshRenderer = meshRenderer;
        hit.distance = distance;

        hits.push_back(hit);
    }

    std::sort(hits.begin(), hits.end(),
        [](const GameObjectPickHit& firstHit, const GameObjectPickHit& secondHit)
        {
            return firstHit.distance < secondHit.distance;
        }
    );

    return hits;
}

bool ModuleScene::pickGameObject(const Ray& worldRay, GameObjectPickHit& outHit)
{
    std::vector<GameObjectPickHit> hits = collectAABBHits(worldRay);

    if (hits.empty())
    {
        return false;
    }

    GameObjectPickHit closestTriangleHit;

    for (const GameObjectPickHit& hit : hits)
    {
        if (!hit.gameObject || !hit.meshRenderer)
        {
            continue;
        }

        float triangleDistance = FLT_MAX;
        Vector3 triangleHitPoint = Vector3::Zero;

        if (!ScenePicking::intersectMeshRendererTriangles(hit.meshRenderer, worldRay, triangleDistance, triangleHitPoint))
        {
            continue;
        }

        if (triangleDistance < closestTriangleHit.distance)
        {
            closestTriangleHit.gameObject = hit.gameObject;
            closestTriangleHit.meshRenderer = hit.meshRenderer;
            closestTriangleHit.hitPoint = triangleHitPoint;
            closestTriangleHit.distance = triangleDistance;
        }
    }

    if (!closestTriangleHit.gameObject)
    {
        return false;
    }

    outHit = closestTriangleHit;
    return true;
}
#pragma endregion

#pragma region Systems
void ModuleScene::initializeRuntimeSceneSystems()
{
    if (app->getCurrentEngineState() != ENGINE_STATE::PLAYING)
    {
        return;
    }

    if (m_scene == nullptr)
    {
        return;
    }

    m_scene->registerAllTriggersInScene();
}

void ModuleScene::clearRuntimeSceneSystems()
{
    if (m_scene == nullptr)
    {
        return;
    }

    m_scene->clearTriggers();
}
#pragma endregion