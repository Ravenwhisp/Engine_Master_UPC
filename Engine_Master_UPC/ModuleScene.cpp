#include "Globals.h"
#include "ModuleScene.h"

#include "Application.h"
#include "Settings.h"
#include "ModuleNavigation.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"

#include "Scene.h"
#include "Quadtree.h"
#include "SceneSerializer.h"
#include "SceneSnapshot.h"

#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "CameraComponent.h"
#include "MeshRenderer.h"
#include "LightComponent.h"
#include "ScriptComponent.h"

#include <unordered_map>

using namespace DirectX::SimpleMath;

ModuleScene::ModuleScene()
{
    m_sceneSerializer = std::make_unique<SceneSerializer>();
    m_scene = std::make_unique<Scene>();
    m_quadtree = std::make_unique<Quadtree>();
}

ModuleScene::~ModuleScene() = default;


void ModuleScene::requestSceneChange(const std::string& sceneName)
{
    m_pendingSceneLoad = sceneName;
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
  
    m_scene->update();
    m_quadtree->update();
}

bool ModuleScene::cleanUp()
{
    m_scene.reset();
    m_quadtree.reset();
    m_sceneSerializer.reset();

    return true;
}
#pragma endregion

#pragma region Caches
void ModuleScene::rebuildComponentCaches()
{
    m_meshRenderers.clear();
    m_lightComponents.clear();
    m_scriptComponents.clear();


    for (GameObject* go : m_scene->getAllGameObjects())
    {
        if (!go->GetActive())
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
    m_sceneSerializer->SaveScene(m_scene.get());
}

bool ModuleScene::loadScene(const std::string& sceneName)
{
    auto newScene = m_sceneSerializer->LoadScene(sceneName);

    if (!newScene)
    {
        DEBUG_ERROR("[ModuleScene] Failed to load scene: %s", sceneName.c_str());
        return false;
    }

    m_scene = std::move(newScene);
    m_scene->setName(sceneName.c_str());

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
    snapshot.applyTo(*m_scene.get());

    m_quadtree = std::make_unique<Quadtree>();
    m_quadtree->init(m_scene.get());
}
#pragma endregion

