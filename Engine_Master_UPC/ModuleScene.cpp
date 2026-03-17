#include "Globals.h"
#include "ModuleScene.h"

#include "Application.h"
#include "ModuleNavigation.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"

#include "Scene.h"
#include "SceneSerializer.h"

#include "Quadtree.h"

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
rapidjson::Value ModuleScene::getJSON(rapidjson::Document& domTree)
{
    rapidjson::Value sceneInfo(rapidjson::kObjectType);

    sceneInfo.AddMember("SkyBox", getSkyBoxJSON(domTree), domTree.GetAllocator());
    sceneInfo.AddMember("Lighting", getLightingJSON(domTree), domTree.GetAllocator());

    uint64_t defaultCameraOwnerUid = 0;
    auto defaultCamera = m_scene->getDefaultCamera();
    if (defaultCamera != nullptr) 
    {
        GameObject* owner = defaultCamera->getOwner();
        defaultCameraOwnerUid = (uint64_t)owner->GetID();
    }

    sceneInfo.AddMember("DefaultCameraOwnerUID", defaultCameraOwnerUid, domTree.GetAllocator());

    // GameObjects serialization //
    {
        rapidjson::Value gameObjectsData(rapidjson::kArrayType);
        auto rootObjects = m_scene->getRootObjects();
        for (GameObject* root : rootObjects)
        {
            serializeWindowHierarchy(root, gameObjectsData, domTree);
        }

        sceneInfo.AddMember("GameObjects", gameObjectsData, domTree.GetAllocator());
    }

    return sceneInfo;
}

void ModuleScene::serializeWindowHierarchy(GameObject* gameObject, rapidjson::Value& gameObjectsData, rapidjson::Document& domTree)
{
    gameObjectsData.PushBack(gameObject->getJSON(domTree), domTree.GetAllocator());

    for (GameObject* child : gameObject->GetTransform()->getAllChildren())
    {
        serializeWindowHierarchy(child, gameObjectsData, domTree);
    }
}

rapidjson::Value ModuleScene::getLightingJSON(rapidjson::Document& domTree)
{
    rapidjson::Value lightingInfo(rapidjson::kObjectType);

    auto lighting = m_scene->GetLightingSettings();
    {
        rapidjson::Value ambientColorData(rapidjson::kArrayType);
        ambientColorData.PushBack(lighting.ambientColor.x, domTree.GetAllocator());
        ambientColorData.PushBack(lighting.ambientColor.y, domTree.GetAllocator());
        ambientColorData.PushBack(lighting.ambientColor.z, domTree.GetAllocator());

        lightingInfo.AddMember("AmbientColor", ambientColorData, domTree.GetAllocator());
    }

    lightingInfo.AddMember("AmbientIntensity", lighting.ambientIntensity, domTree.GetAllocator());

    return lightingInfo;
}

rapidjson::Value ModuleScene::getSkyBoxJSON(rapidjson::Document& domTree)
{
    rapidjson::Value skyboxInfo(rapidjson::kObjectType);

    auto skybox = m_scene->getSkyBoxSettings();

    skyboxInfo.AddMember("Enabled", skybox.enabled, domTree.GetAllocator());
    skyboxInfo.AddMember("CubemapAssetId", (uint64_t)skybox.cubemapAssetId, domTree.GetAllocator());

    return skyboxInfo;
}

void ModuleScene::saveScene()
{
    rapidjson::Document domTree;
    domTree.SetObject();

    {
        rapidjson::Value name(m_scene->getName(), domTree.GetAllocator());
        domTree.AddMember(name, getJSON(domTree), domTree.GetAllocator());
    }

    m_sceneSerializer->SaveScene(m_scene->getName(), domTree);
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

void ModuleScene::requestSceneChange(const std::string& sceneName)
{
    m_pendingSceneLoad = sceneName;
}
#pragma endregion

