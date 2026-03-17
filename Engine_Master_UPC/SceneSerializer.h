#pragma once
#include <memory>
#include <string>
#include <rapidjson/document.h>

class Scene;

class SceneSerializer
{
public:
    SceneSerializer();
    ~SceneSerializer() = default;

    bool SaveScene(std::string sceneName, rapidjson::Document& domTree);
    static std::unique_ptr<Scene> LoadScene(const std::string& sceneName);

private:
    static std::string BuildScenePath(const std::string& sceneName);

    static bool LoadFromJSON(Scene& scene, const rapidjson::Value& sceneJson);

    static bool LoadSkybox(Scene& scene, const rapidjson::Value& json);
    static void LoadLighting(Scene& scene, const rapidjson::Value& json);

    static void CreateGameObjects(Scene& scene, const rapidjson::Value& array, std::unordered_map<uint64_t, class GameObject*>& uidToGo, std::vector<std::pair<uint64_t, uint64_t>>& hierarchy);
    static void LinkHierarchy(Scene& scene, const std::unordered_map<uint64_t, class GameObject*>& uidToGo, const std::vector<std::pair<uint64_t, uint64_t>>& hierarchy);

    static void FixReferences(Scene& scene);
    static void ResolveDefaultCamera(Scene& scene, const rapidjson::Value& json);
};