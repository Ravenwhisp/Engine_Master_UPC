#include "Globals.h"
#include "SceneSerializer.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include <cstdio>

#include "Scene.h"
#include "SceneLightingSettings.h"
#include "SkyBoxSettings.h"

#include "GameObject.h"
#include "Transform.h"
#include "Component.h"
#include "CameraComponent.h"

#include "Application.h"
#include "Settings.h"

#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include "SceneReferenceResolver.h"
#include "MD5Fwd.h"


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

std::string SceneSerializer::BuildScenePath(const std::string& sceneName)
{
    return std::string(SCENE_FOLDER) + sceneName + std::string(SCENE_FILE_EXTENSION);
}

#pragma region Save
bool SceneSerializer::SaveScene(const Scene* scene)
{
    if (!scene)
    {
        DEBUG_ERROR("[SceneSerializer] SaveScene: scene is null");
        return false;
    }

    std::string sceneName = scene->getName();

    if (sceneName.empty())
    {
        DEBUG_ERROR("[SceneSerializer] Scene name cannot be empty");
        return false;
    }

    const std::string path = BuildScenePath(sceneName);

    DEBUG_LOG("[SceneSerializer] Saving scene: %s", sceneName.c_str());
    DEBUG_LOG("[SceneSerializer] Path: %s", path.c_str());

    rapidjson::Document domTree;
    domTree.SetObject();

    rapidjson::Value sceneValue = getJSON(domTree, scene);

    domTree.Swap(sceneValue);

    FILE* fileOpened = std::fopen(path.c_str(), "wb");
    if (!fileOpened)
    {
        DEBUG_ERROR("[SceneSerializer] Failed to open file for writing: %s", path.c_str());
        return false;
    }

    char writeBuffer[65536];
    rapidjson::FileWriteStream stream(fileOpened, writeBuffer, sizeof(writeBuffer));

    rapidjson::Writer<rapidjson::FileWriteStream> writer(stream);
    domTree.Accept(writer);

    std::fclose(fileOpened);

    DEBUG_LOG("[SceneSerializer] Scene saved successfully");

    return true;
}

rapidjson::Value SceneSerializer::getJSON(rapidjson::Document& domTree, const Scene* scene)
{
    rapidjson::Value sceneInfo(rapidjson::kObjectType);

    sceneInfo.AddMember("Scene", rapidjson::Value(scene->getName(), domTree.GetAllocator()), domTree.GetAllocator());

    const std::string& version = app->getSettings()->engine.version;
    sceneInfo.AddMember("Version", rapidjson::Value(version.c_str(), domTree.GetAllocator()),domTree.GetAllocator());

    sceneInfo.AddMember("SkyBox", getSkyBoxJSON(domTree, scene), domTree.GetAllocator());
    sceneInfo.AddMember("Lighting", getLightingJSON(domTree, scene), domTree.GetAllocator());

    uint64_t defaultCameraOwnerUid = 0;
    auto defaultCamera = scene->getDefaultCamera();
    if (defaultCamera != nullptr)
    {
        GameObject* owner = defaultCamera->getOwner();
        defaultCameraOwnerUid = (uint64_t)owner->GetID();
    }

    sceneInfo.AddMember("DefaultCameraOwnerUID", defaultCameraOwnerUid, domTree.GetAllocator());

    // GameObjects serialization //
    {
        rapidjson::Value gameObjectsData(rapidjson::kArrayType);
        auto rootObjects = scene->getRootObjects();
        for (GameObject* root : rootObjects)
        {
            serializeWindowHierarchy(root, gameObjectsData, domTree, scene);
        }

        sceneInfo.AddMember("GameObjects", gameObjectsData, domTree.GetAllocator());
    }

    return sceneInfo;
}

void SceneSerializer::serializeWindowHierarchy(GameObject* gameObject, rapidjson::Value& gameObjectsData, rapidjson::Document& domTree, const Scene* scene)
{
    gameObjectsData.PushBack(gameObject->getJSON(domTree), domTree.GetAllocator());

    for (GameObject* child : gameObject->GetTransform()->getAllChildren())
    {
        serializeWindowHierarchy(child, gameObjectsData, domTree, scene);
    }
}

rapidjson::Value SceneSerializer::getLightingJSON(rapidjson::Document& domTree, const Scene* scene)
{
    rapidjson::Value lightingInfo(rapidjson::kObjectType);

    const SceneLightingSettings& lighting = scene->getLightingSettings();
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

rapidjson::Value SceneSerializer::getSkyBoxJSON(rapidjson::Document& domTree, const Scene* scene)
{
    rapidjson::Value skyboxInfo(rapidjson::kObjectType);

    const SkyBoxSettings& skybox = scene->getSkyBoxSettings();

    skyboxInfo.AddMember("Enabled", skybox.enabled, domTree.GetAllocator());
    skyboxInfo.AddMember("CubemapAssetId", rapidjson::Value(skybox.cubemapAssetId.c_str(), domTree.GetAllocator()), domTree.GetAllocator());

    return skyboxInfo;
}

#pragma endregion

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

    if (!doc.IsObject())
    {
        DEBUG_ERROR("[SceneSerializer] Scene JSON root is not an object");
        return nullptr;
    }

    auto scene = std::make_unique<Scene>();

    if (!LoadFromJSON(*scene, doc))
    {
        DEBUG_ERROR("[SceneSerializer] Failed to load scene from JSON");
        return nullptr;
    }

    DEBUG_LOG("[SceneSerializer] Scene loaded successfully: %s", scene->getName());
    return scene;
}

bool SceneSerializer::LoadFromJSON(Scene& scene, const rapidjson::Value& json)
{
    if (json.HasMember("Scene") && json["Scene"].IsString())
    {
        scene.setName(json["Scene"].GetString());
    }
    else
    {
        DEBUG_WARN("[SceneSerializer] Scene field missing or invalid");
    }

    if (!ValidateVersion(json))
    {
        return false;
    }

    if (!LoadSkybox(scene, json))
    {
        DEBUG_WARN("[SceneSerializer] Skybox missing or invalid");
    }

    LoadLighting(scene, json);

    if (!json.HasMember("GameObjects") || !json["GameObjects"].IsArray())
    {
        DEBUG_ERROR("[SceneSerializer] GameObjects missing or invalid");
        return false;
    }

    size_t goNumber = json["GameObjects"].Size();
    std::vector<uint64_t> uidSet;
    uidSet.reserve(goNumber);
    std::vector<GameObject*> goSet;
    goSet.reserve(goNumber);

    std::vector<std::pair<uint64_t, uint64_t>> hierarchy;
    hierarchy.reserve(goNumber);

    CreateGameObjects(scene, json["GameObjects"], uidSet, goSet, hierarchy);
    LinkHierarchy(scene, uidSet, goSet, hierarchy);

    FixReferences(scene);
    ResolveDefaultCamera(scene, json);

    return true;
}

bool SceneSerializer::ValidateVersion(const rapidjson::Value& json)
{
    if (!json.HasMember("Version") || !json["Version"].IsString())
    {
        DEBUG_ERROR("[SceneSerializer] Version field missing or invalid");
        return false;
    }

    const std::string fileVersion = json["Version"].GetString();
    const std::string& engineVersion = app->getSettings()->engine.version;

    if (fileVersion != engineVersion)
    {
        DEBUG_ERROR("[SceneSerializer] Scene version mismatch. File version: %s | Engine version: %s", fileVersion.c_str(), engineVersion.c_str());
        return false;
    }

    return true;
}

bool SceneSerializer::LoadSkybox(Scene& scene, const rapidjson::Value& json)
{
    if (!json.HasMember("SkyBox"))
    {
        DEBUG_WARN("[SceneSerializer] Skybox json object missing.");
        return false;
    }

    SkyBoxSettings& skyboxSettings = scene.getSkyBoxSettings();
    const auto& data = json["SkyBox"];

    if (data.HasMember("Enabled") && data["Enabled"].IsBool())
    {
        skyboxSettings.enabled = data["Enabled"].GetBool();
    }
    else
    {
        DEBUG_WARN("[SceneSerializer] Skybox Enabled missing");
    }

    if (data.HasMember("CubemapAssetId") && data["CubemapAssetId"].IsString())
    {
        skyboxSettings.cubemapAssetId = (MD5Hash)data["CubemapAssetId"].GetString();
    }
    else
    {
        DEBUG_WARN("[SceneSerializer] Skybox Cubemap missing");
    }

    return true;
}

void SceneSerializer::LoadLighting(Scene& scene, const rapidjson::Value& json)
{
    if (!json.HasMember("Lighting"))
    {
        DEBUG_WARN("[SceneSerializer] Lighting missing");
        return;
    }

    SceneLightingSettings& lighting = scene.getLightingSettings();
    const auto& data = json["Lighting"];

    if (data.HasMember("AmbientColor") && data["AmbientColor"].IsArray() && data["AmbientColor"].Size() == 3)
    {
        const auto& color = data["AmbientColor"].GetArray();
        lighting.ambientColor = {
            color[0].GetFloat(),
            color[1].GetFloat(),
            color[2].GetFloat()
        };
    }
    else
    {
        DEBUG_WARN("[SceneSerializer] Invalid AmbientColor");
    }

    if (data.HasMember("AmbientIntensity")) {
        lighting.ambientIntensity = data["AmbientIntensity"].GetFloat();
    }
    else {
        DEBUG_WARN("[SceneSerializer] Invalid AmbientIntensity");
    }
}

void SceneSerializer::CreateGameObjects(
    Scene& scene,
    const rapidjson::Value& array,
    std::vector<uint64_t>& uidSet,
    std::vector<GameObject*>& goSet,
    std::vector<std::pair<uint64_t, uint64_t>>& hierarchy)
{
    for (auto& json : array.GetArray())
    {
        const uint64_t uid = json["UID"].GetUint64();
        const uint64_t transformUid = json["Transform"]["UID"].GetUint64();

        GameObject* go = scene.createGameObjectWithUID((UID)uid, (UID)transformUid);

        uint64_t parentUid = 0;
        go->deserializeJSON(json, parentUid);

        uidSet.push_back(uid);
        goSet.push_back(go);

        hierarchy.emplace_back(uid, parentUid);
    }
}

void SceneSerializer::LinkHierarchy(
    Scene& scene,
    std::vector<uint64_t>& uidSet,
    std::vector<GameObject*>& goSet,
    const std::vector<std::pair<uint64_t, uint64_t>>& hierarchy)
{
    int childIdx, parentIdx;
    for (const auto& [childUid, parentUid] : hierarchy)
    {
        if (parentUid == 0)
        {
            continue;
        }

        childIdx = -1;
        parentIdx = -1;

        for (int i = 0; i < uidSet.size() && (childIdx == -1  || parentIdx == -1); i++) {
            if (uidSet[i] == childUid) {
                childIdx = i;
            }

            if (uidSet[i] == parentUid) {
                parentIdx = i;
            }
        }

        if (childIdx == -1 || parentIdx == -1)
        {
            DEBUG_ERROR("Hierarchy link failed: UID not found");
            continue;
        }

        GameObject* child = goSet[childIdx];
        GameObject* parent = goSet[parentIdx];

        child->GetTransform()->setRoot(parent->GetTransform());
        parent->GetTransform()->addChild(child);

        scene.removeFromRootList(child);
    }
}

void SceneSerializer::FixReferences(Scene& scene)
{
    SceneReferenceResolver resolver;

    for (GameObject* obj : scene.getAllGameObjects())
    {
        resolver.registerGameObject(obj, obj);

        for (Component* c : obj->GetAllComponents())
        {
            resolver.registerComponent(c->getID(), c);
        }
    }

    for (GameObject* obj : scene.getAllGameObjects())
    {
        for (Component* c : obj->GetAllComponents())
        {
            c->fixReferences(resolver);
        }
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
#pragma endregion
