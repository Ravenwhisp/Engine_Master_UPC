#include "Globals.h"
#include "ModuleScene.h"

#include "Application.h"
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

#include <unordered_map>

using namespace DirectX::SimpleMath;

ModuleScene::ModuleScene()
{
    m_sceneSerializer = std::make_unique<SceneSerializer>();
    m_scene = std::make_unique<Scene>();
    m_quadtree = std::make_unique<Quadtree>();
}

ModuleScene::~ModuleScene() = default;

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

std::vector<MeshRenderer*> ModuleScene::getAllMeshRenderers() const
{
    std::vector<MeshRenderer*> result;
    std::vector<GameObject*> objects;

    bool useCulling = false;
    if (useCulling && m_scene->getDefaultCamera())
    {
        objects = m_quadtree->query();
    }
    else
    {
        objects = m_scene->getAllGameObjects();
    }

    for (GameObject* go : objects)
    {
        if (!go->GetActive())
            continue;

        auto mesh = go->GetComponentAs<MeshRenderer>(ComponentType::MODEL);

        if (mesh && mesh->hasMeshes())
            result.push_back(mesh);
    }

    return result;
}

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

    return true;
}

#pragma endregion

void ModuleScene::requestSceneChange(const std::string& sceneName)
{
    m_pendingSceneLoad = sceneName;
}

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

