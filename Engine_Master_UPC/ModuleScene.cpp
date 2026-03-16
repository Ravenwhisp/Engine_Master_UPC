#include "Globals.h"

#include "ModuleScene.h"
#include "LightComponent.h"
#include <CameraComponent.h>
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleEditor.h"
#include "Settings.h"
#include "GameObject.h"
#include "UID.h"

#include "Quadtree.h"
#include "SceneSerializer.h"
#include "ModuleNavigation.h"

#include "Scene.h";

#include <queue>
#include <limits>

using namespace DirectX::SimpleMath;

ModuleScene::ModuleScene()
{
    m_sceneSerializer = std::make_unique<SceneSerializer>();
    m_scene = std::make_unique<Scene>();
}

ModuleScene::~ModuleScene() = default;

Scene* ModuleScene::getScene()
{
    return m_scene.get();
}

#pragma region GameLoop
bool ModuleScene::init()
{
    m_scene->init();
    

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
}


void ModuleScene::render(ID3D12GraphicsCommandList* commandList)
{
    m_scene->render(commandList);
}

bool ModuleScene::cleanUp()
{
    m_scene->cleanUp();
    
    m_sceneSerializer.reset();
    return true;
}
#pragma endregion

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

bool ModuleScene::loadFromJSON(const rapidjson::Value& sceneJson) 
{
    const auto& gameObjectsArray = sceneJson["GameObjects"].GetArray();

    m_scene->clearScene();

    if (!loadSceneSkyBox(sceneJson))
    {
		DEBUG_LOG("Failed to load skybox settings from scene JSON. Possible wrong version of scene data.");
        return false;
    }
    loadSceneLighting(sceneJson);

    // Create all objects and components
    std::unordered_map<uint64_t, GameObject*> uidToGo;
    std::vector<std::pair<uint64_t, uint64_t>> childToParent;

    for (auto& gameObjectJson : gameObjectsArray)
    {
        const uint64_t uid = gameObjectJson["UID"].GetUint64();
        const uint64_t transformUid = gameObjectJson["Transform"]["UID"].GetUint64();
        GameObject* gameObject = m_scene->createGameObjectWithUID((UID)uid, (UID)transformUid);

        uint64_t parentUid = 0;
        gameObject->deserializeJSON(gameObjectJson, parentUid);

        uidToGo[uid] = gameObject;
        childToParent.push_back({ uid, parentUid });
    }

    // Parent Child linking
    for (const auto& pair : childToParent)
    {
        const uint64_t childUid = pair.first;
        const uint64_t parentUid = pair.second;

        if (parentUid == 0) {
            continue;
        }

        GameObject* child = uidToGo[childUid];
        GameObject* parent = uidToGo[parentUid];

        child->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(child);

        m_scene->removeFromRootList(child);
    }

    fixLoadedSceneReferences();
    resolveDefaultCamera(sceneJson);
    m_scene->applySkyBoxToRenderer();

    return true;
}

bool ModuleScene::loadSceneSkyBox(const rapidjson::Value& sceneJson)
{
    if (!sceneJson.HasMember("SkyBox"))
    {
        return false;
    }

    auto skybox = m_scene->getSkyBoxSettings();
    const auto& skyboxJson = sceneJson["SkyBox"];

    if (!skyboxJson.HasMember("Enabled") && skyboxJson["Enabled"].IsBool())
    {
        return false;
    }
    skybox.enabled = skyboxJson["Enabled"].GetBool();

    if (!skyboxJson.HasMember("CubemapAssetId") && skyboxJson["CubemapAssetId"].IsUint64())
    {
        return false;
    }
    skybox.cubemapAssetId = (UID)skyboxJson["CubemapAssetId"].GetUint64();

    return true;
}

bool ModuleScene::loadSceneLighting(const rapidjson::Value& sceneJson) 
{
    auto lighting = m_scene->GetLightingSettings();
    const auto& lightingJson = sceneJson["Lighting"];

    const auto& color = lightingJson["AmbientColor"].GetArray();
    lighting.ambientColor = Vector3(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat());

    lighting.ambientIntensity = lightingJson["AmbientIntensity"].GetFloat();
    return true;
}

void ModuleScene::fixLoadedSceneReferences()
{
    std::unordered_map<UID, Component*> componentMap;

    auto allObjects = m_scene->getAllGameObjects();
    for (const auto& obj : allObjects)
    {
        for (Component* component : obj->GetAllComponents())
        {
            componentMap[component->getID()] = component;
        }
    }

    for (const auto& obj : allObjects)
    {
        for (Component* component : obj->GetAllComponents())
        {
            component->fixReferences(componentMap);
        }
    }
}

void ModuleScene::resolveDefaultCamera(const rapidjson::Value& sceneJson) 
{
    m_scene->setDefaultCamera(nullptr);

    if (sceneJson.HasMember("DefaultCameraOwnerUID"))
    {
        const uint64_t cameraOwnerUID = sceneJson["DefaultCameraOwnerUID"].GetUint64();

        if (cameraOwnerUID != 0)
        {
            GameObject* ownerGameObject = m_scene->findGameObjectByUID((UID)cameraOwnerUID);
            CameraComponent* cameraComponent = ownerGameObject->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
            m_scene->setDefaultCamera( cameraComponent );
        }
    }
}


void ModuleScene::saveScene()
{
    rapidjson::Document domTree;
    domTree.SetObject();

    {
        rapidjson::Value name(m_scene->getName(), domTree.GetAllocator()); // copy string m_name
        domTree.AddMember(name, getJSON(domTree), domTree.GetAllocator());
    }

    m_sceneSerializer->SaveScene(m_scene->getName(), domTree);
}

bool ModuleScene::loadScene(const std::string& sceneName)
{
    m_scene->resetQuadtree();
    m_scene->clearScene();
    const bool fileExists = m_sceneSerializer->LoadScene(sceneName);
    if (!fileExists) 
    {
        return false;
    }

    m_scene->setName(sceneName.c_str());

    const char* s = sceneName.c_str();
    if (app->getModuleNavigation()->loadNavMeshForScene(s))
    {
        DEBUG_LOG("LOADED NavMesh for scene: %s\n", s);
    }
    else
    {
        DEBUG_ERROR("CANNOT load NavMesh for this scene\n");
    }
   
    return true;
}

void ModuleScene::requestSceneChange(const std::string& sceneName)
{
    m_pendingSceneLoad = sceneName;
}
#pragma endregion

