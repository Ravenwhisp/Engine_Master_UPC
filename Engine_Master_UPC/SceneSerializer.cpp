#include "Globals.h"
#include "SceneSerializer.h"

#include <filesystem>
#include <iostream>

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "Application.h"
#include "SceneModule.h"
#include "GameObject.h"
#include "Transform.h"
#include "Component.h"
#include "ComponentType.h"

#include <rapidjson/document.h>


constexpr std::string_view LOG_TAG = "SceneSerializer";
constexpr std::string_view SCENE_FILE_EXTENSION = ".scene";
constexpr std::string_view SCENE_FOLDER = "Assets/Scenes/";

SceneSerializer::SceneSerializer()
{
    if (!std::filesystem::exists(SCENE_FOLDER))
    {
        if (std::filesystem::create_directories(SCENE_FOLDER))
        {
            LOG("Created scene folder: %s\n", SCENE_FOLDER.data());
        }
        else
        {
            LOG("Failed to create scene folder: %s\n", SCENE_FOLDER.data());
        }
    }
}


SceneSerializer::~SceneSerializer()
{
}

bool SceneSerializer::SaveScene(std::string sceneName)
{
    if (sceneName.empty())
    {
        LOG("Scene name cannot be empty.\n");
        return false;
	}

    return true;
}


bool SceneSerializer::LoadScene(std::string sceneName)
{
    if (sceneName.empty()) {
        return false;
    }

    const std::string path = std::string(SCENE_FOLDER) + sceneName + std::string(SCENE_FILE_EXTENSION);

    // Read file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string json = ss.str();

    // Parse JSON
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError()) {
        return false;
    }

    const auto& sceneJson = doc["Scene"];
    const auto& gameObjectsArray = sceneJson["GameObjects"].GetArray();

    SceneModule* sceneModule = app->getSceneModule();
    sceneModule->clearScene();

    loadSceneSkybox(sceneModule, sceneJson);
    loadSceneLighting(sceneModule, sceneJson);

    // Create all objects and components
    std::unordered_map<uint64_t, GameObject*> uidToGo;
    std::unordered_map<uint64_t, uint64_t> childToParent;

    for (auto& gameObjectJson : gameObjectsArray)
    {
        const uint64_t uid = gameObjectJson["UID"].GetUint64();
        GameObject* gameObject = sceneModule->createGameObjectWithUID((UID)uid);

        uint64_t parentUid = 0;
        gameObject->deserializeJSON(gameObjectJson, parentUid);

        uidToGo[uid] = gameObject;
        childToParent[uid] = parentUid;
    }

    // Parent Child linking
    for (const auto& childAndParent : childToParent)
    {
        const uint64_t childUid = childAndParent.first;
        const uint64_t parentUid = childAndParent.second;

        if (parentUid == 0) {
            continue;
        }

        GameObject* child = uidToGo[childUid];
        GameObject* parent = uidToGo[parentUid];

        child->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(child);

        sceneModule->detachGameObject(child);
    }

    for (GameObject* rootGameObject : sceneModule->getAllGameObjects())
        rootGameObject->init();

    sceneModule->applySkyboxToRenderer();

    return true;
}

bool SceneSerializer::loadSceneSkybox(SceneModule* sceneModule, const rapidjson::Value& sceneJson) {
    auto& skybox = sceneModule->getSkyboxSettings();

    const auto& skyboxJson = sceneJson["Skybox"];
    skybox.enabled = skyboxJson["Enabled"].GetBool();

    const char* pathStr = skyboxJson["Path"].GetString();
    strcpy_s(skybox.path, 260, pathStr);
    
    return true;
}

bool SceneSerializer::loadSceneLighting(SceneModule* sceneModule, const rapidjson::Value& sceneJson) {
    auto& lighting = sceneModule->GetLightingSettings();

    const auto& lightingJson = sceneJson["Lighting"];

    const auto& color = lightingJson["AmbientColor"].GetArray();
    lighting.ambientColor = Vector3(color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat());

    lighting.ambientIntensity = lightingJson["AmbientIntensity"].GetFloat();

    return true;
}
