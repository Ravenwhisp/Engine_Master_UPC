#include "Globals.h"
#include "ModuleScene.h"

#include "Application.h"
#include "Settings.h"
#include "ModuleNavigation.h"
#include "ModuleEditor.h"

#include "Scene.h"
#include "Quadtree.h"
#include "SceneSerializer.h"
#include "SceneSnapshot.h"
#include "ModuleAssets.h"

#include "GameObject.h"
#include "MeshRenderer.h"
#include "LightComponent.h"
#include "ScriptComponent.h"

ModuleScene::ModuleScene()
{
    m_sceneSerializer = std::make_unique<SceneSerializer>();
    m_scene = std::make_unique<Scene>(GenerateUID());
    m_quadtree = std::make_unique<Quadtree>();
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
    m_quadtree->init(m_scene.get());
    
    return true;
}

void ModuleScene::update()
{
    if (!m_pendingSceneLoad.empty())
    {
        loadScene(m_pendingSceneLoad);
        m_pendingSceneLoad.clear();
    }  

    if(m_pendingScene)
    {
		loadScene(m_pendingScene);
		m_pendingScene.reset();
	}

    syncQuadtreeWithSettings();
  
    m_scene->update();
    m_quadtree->update();
}

bool ModuleScene::cleanUp()
{
    clearComponentCaches();

    m_scene.reset();
    m_quadtree.reset();
    m_sceneSerializer.reset();

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

const std::vector<MeshRenderer*> ModuleScene::getVisibleMeshRenderers()
{
    if (app->getSettings()->frustumCulling.debugFrustumCulling)
    {
        std::vector<MeshRenderer*> visibleMeshRenderers = {};
        for (GameObject* gO : app->getModuleScene()->getQuadtree()->query())
        {
            MeshRenderer* renderer = gO->GetComponentAs<MeshRenderer>(ComponentType::MODEL);
            if (renderer)
            {
                visibleMeshRenderers.push_back(renderer);
            }
        }
        return visibleMeshRenderers;
    }
    return app->getModuleScene()->getMeshRenderers();
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
#pragma endregion

#pragma region Persistence
void ModuleScene::saveScene()
{
    app->getModuleAssets()->save(*m_scene.get());
}

bool ModuleScene::loadScene(const std::string& sceneName)
{
    clearComponentCaches();

    auto newScene = m_sceneSerializer->LoadScene(sceneName);

    if (!newScene)
    {
        DEBUG_ERROR("[ModuleScene] Failed to load scene: %s", sceneName.c_str());
        return false;
    }

    m_scene = std::move(newScene);
    m_scene->setName(sceneName.c_str());
    m_scene->markDirty();

    m_quadtree = std::make_unique<Quadtree>();
    m_quadtree->init(m_scene.get());

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
    m_quadtree->build();
#endif

    rebuildComponentCaches();
    return true;
}

bool ModuleScene::loadScene(std::shared_ptr<Scene> scene)
{
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

    m_quadtree = std::make_unique<Quadtree>();
    m_quadtree->init(m_scene.get());

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
    m_quadtree->build();
#endif

    rebuildComponentCaches();
    return true;
}

#pragma endregion

#pragma region Snapshot
SceneSnapshot* ModuleScene::takeSnapshot() const
{
    SceneSnapshot * sceneSnapshot = new SceneSnapshot();
    sceneSnapshot->init(*m_scene.get());

    return sceneSnapshot;
}

void ModuleScene::loadFromSnapshot(SceneSnapshot& snapshot)
{
    clearComponentCaches();

    snapshot.applyTo(*m_scene.get());
    m_scene->markDirty();

    m_quadtree = std::make_unique<Quadtree>();
    m_quadtree->init(m_scene.get());

    rebuildComponentCaches();
}
#pragma endregion

#pragma region Quadtree
void ModuleScene::syncQuadtreeWithSettings()
{
    if (!m_quadtree)
    {
        return;
    }

    const bool shouldShowQuadtree = app->getSettings()->sceneEditor.showQuadTree;

    if (shouldShowQuadtree && !m_quadtree->getIsBuilded())
    {
        m_quadtree->build();
    }
    else if (!shouldShowQuadtree && m_quadtree->getIsBuilded())
    {
        m_quadtree->clear();
    }
}
#pragma endregion
