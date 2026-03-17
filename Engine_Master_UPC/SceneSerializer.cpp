#include "Globals.h"
#include "SceneSerializer.h"

#include <filesystem>
#include <iostream>

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "Application.h"
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "Component.h"
#include "ComponentType.h"
#include "CameraComponent.h"

#include <rapidjson/document.h>
#include "rapidjson/filewritestream.h"
#include <rapidjson/writer.h>


constexpr std::string_view LOG_TAG = "SceneSerializer";
constexpr std::string_view SCENE_FILE_EXTENSION = ".scene";
constexpr std::string_view SCENE_FOLDER = "Assets/Scenes/";

SceneSerializer::SceneSerializer()
{
    if (!std::filesystem::exists(SCENE_FOLDER))
    {
        if (std::filesystem::create_directories(SCENE_FOLDER))
        {
            DEBUG_LOG("[SceneSerializer] Scene folder created");
        }
        else
        {
            DEBUG_ERROR("[SceneSerializer] Failed to create scene folder");
        }
    }
}

bool SceneSerializer::SaveScene(std::string sceneName, rapidjson::Document& domTree)
{
    if (sceneName.empty())
    {
        DEBUG_LOG("Scene name cannot be empty.\n");
        return false;
    }

    const std::string path = std::string(SCENE_FOLDER) + sceneName + std::string(SCENE_FILE_EXTENSION);

    // Save file //

    FILE* fileOpened = std::fopen(path.c_str(), "wb"); // w for writing, b disables special handling of '\n' and '\x1A'
    if (!fileOpened)
    {
        DEBUG_ERROR("Error opening file");
        return false;
    }

    // Create a FileWriteStream
    char writeBuffer[65536];
    rapidjson::FileWriteStream stream(fileOpened, writeBuffer, sizeof(writeBuffer));

    // Write JSON to file
    rapidjson::Writer<rapidjson::FileWriteStream> writer(stream);
    domTree.Accept(writer);

    // Close file
    std::fclose(fileOpened);

    return true;
}

#pragma region Load
std::unique_ptr<Scene> SceneSerializer::LoadScene(const std::string& sceneName)
{
    if (sceneName.empty())
    {
        DEBUG_ERROR("[SceneSerializer] Scene name is empty");
        return nullptr;
    }

    const std::string path = BuildScenePath(sceneName);

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        DEBUG_ERROR("[SceneSerializer] Scene file not found: %s", path.c_str());
        return nullptr;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    rapidjson::Document doc;
    doc.Parse(ss.str().c_str());

    if (doc.HasParseError())
    {
        DEBUG_ERROR("[SceneSerializer] JSON parse error");
        return nullptr;
    }

    if (!doc.HasMember(sceneName.c_str()))
    {
        DEBUG_ERROR("[SceneSerializer] Scene root not found in JSON");
        return nullptr;
    }

    auto scene = std::make_unique<Scene>();

    if (!LoadFromJSON(*scene, doc[sceneName.c_str()]))
    {
        DEBUG_ERROR("[SceneSerializer] Failed to load scene from JSON");
        return nullptr;
    }

    DEBUG_LOG("[SceneSerializer] Scene loaded successfully: %s", sceneName.c_str());
    return scene;
}

bool SceneSerializer::LoadFromJSON(Scene& scene, const rapidjson::Value& json)
{
    if (!LoadSkybox(scene, json))
        DEBUG_WARN("[SceneSerializer] Skybox missing or invalid");

    LoadLighting(scene, json);

    if (!json.HasMember("GameObjects") || !json["GameObjects"].IsArray())
    {
        DEBUG_ERROR("[SceneSerializer] GameObjects missing or invalid");
        return false;
    }

    std::unordered_map<uint64_t, GameObject*> uidToGo;
    std::vector<std::pair<uint64_t, uint64_t>> hierarchy;

    CreateGameObjects(scene, json["GameObjects"], uidToGo, hierarchy);
    LinkHierarchy(scene, uidToGo, hierarchy);

    FixReferences(scene);
    ResolveDefaultCamera(scene, json);

    scene.applySkyBoxToRenderer();

    return true;
}

bool SceneSerializer::LoadSkybox(Scene& scene, const rapidjson::Value& json)
{
    if (!json.HasMember("SkyBox"))
        return false;

    auto& skybox = scene.getSkyBoxSettings();
    const auto& data = json["SkyBox"];

    if (data.HasMember("Enabled") && data["Enabled"].IsBool())
        skybox.enabled = data["Enabled"].GetBool();
    else
        DEBUG_WARN("[SceneSerializer] Skybox Enabled missing");

    if (data.HasMember("CubemapAssetId") && data["CubemapAssetId"].IsUint64())
        skybox.cubemapAssetId = (UID)data["CubemapAssetId"].GetUint64();
    else
        DEBUG_WARN("[SceneSerializer] Skybox Cubemap missing");

    return true;
}

void SceneSerializer::LoadLighting(Scene& scene, const rapidjson::Value& json)
{
    if (!json.HasMember("Lighting"))
    {
        DEBUG_WARN("[SceneSerializer] Lighting missing");
        return;
    }

    auto& lighting = scene.GetLightingSettings();
    const auto& data = json["Lighting"];

    const auto& color = data["AmbientColor"].GetArray();
    lighting.ambientColor = { color[0].GetFloat(), color[1].GetFloat(), color[2].GetFloat() };

    lighting.ambientIntensity = data["AmbientIntensity"].GetFloat();
}

void SceneSerializer::CreateGameObjects(
    Scene& scene,
    const rapidjson::Value& array,
    std::unordered_map<uint64_t, GameObject*>& uidToGo,
    std::vector<std::pair<uint64_t, uint64_t>>& hierarchy)
{
    for (auto& json : array.GetArray())
    {
        const uint64_t uid = json["UID"].GetUint64();
        const uint64_t transformUid = json["Transform"]["UID"].GetUint64();

        GameObject* go = scene.createGameObjectWithUID((UID)uid, (UID)transformUid);

        uint64_t parentUid = 0;
        go->deserializeJSON(json, parentUid);

        uidToGo[uid] = go;
        hierarchy.emplace_back(uid, parentUid);
    }
}

void SceneSerializer::LinkHierarchy(
    Scene& scene,
    const std::unordered_map<uint64_t, GameObject*>& uidToGo,
    const std::vector<std::pair<uint64_t, uint64_t>>& hierarchy)
{
    for (const auto& [childUid, parentUid] : hierarchy)
    {
        if (parentUid == 0)
            continue;

        GameObject* child = uidToGo.at(childUid);
        GameObject* parent = uidToGo.at(parentUid);

        child->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(child);

        scene.removeFromRootList(child);
    }
}

void SceneSerializer::FixReferences(Scene& scene)
{
    std::unordered_map<UID, Component*> map;

    for (auto obj : scene.getAllGameObjects())
    {
        for (Component* c : obj->GetAllComponents())
            map[c->getID()] = c;
    }

    for (auto obj : scene.getAllGameObjects())
    {
        for (Component* c : obj->GetAllComponents())
            c->fixReferences(map);
    }
}

void SceneSerializer::ResolveDefaultCamera(Scene& scene, const rapidjson::Value& json)
{
    scene.setDefaultCamera(nullptr);

    if (!json.HasMember("DefaultCameraOwnerUID"))
        return;

    const uint64_t uid = json["DefaultCameraOwnerUID"].GetUint64();
    if (uid == 0)
        return;

    GameObject* go = scene.findGameObjectByUID((UID)uid);
    if (!go)
    {
        DEBUG_WARN("[SceneSerializer] Default camera owner not found");
        return;
    }

    auto* cam = go->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
    scene.setDefaultCamera(cam);
}

std::string SceneSerializer::BuildScenePath(const std::string& sceneName)
{
    return std::string(SCENE_FOLDER) + sceneName + std::string(SCENE_FILE_EXTENSION);
}
#pragma endregion
