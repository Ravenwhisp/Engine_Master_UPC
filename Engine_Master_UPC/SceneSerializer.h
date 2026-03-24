#pragma once
#include <memory>
#include <string>

class Scene;
class GameObject;

class SceneSerializer
{
public:
    SceneSerializer();
    ~SceneSerializer() = default;

    bool SaveScene(const Scene* scene);
    static std::unique_ptr<Scene> LoadScene(const std::string& sceneName);

private:
    static std::string BuildScenePath(const std::string& sceneName);

#pragma region Save
    static rapidjson::Value getJSON(rapidjson::Document& domTree, const Scene* scene);
    static void serializeWindowHierarchy(GameObject* gameObject, rapidjson::Value& gameObjectsData, rapidjson::Document& domTree, const Scene* scene);
    static rapidjson::Value getLightingJSON(rapidjson::Document& domTree, const Scene* scene);
    static rapidjson::Value getSkyBoxJSON(rapidjson::Document& domTree, const Scene* scene);
#pragma endregion

#pragma region Load
    static bool LoadFromJSON(Scene& scene, const rapidjson::Value& sceneJson);

    static bool LoadSkybox(Scene& scene, const rapidjson::Value& json);
    static void LoadLighting(Scene& scene, const rapidjson::Value& json);

    static void CreateGameObjects(Scene& scene, const rapidjson::Value& array, std::vector<uint64_t>& uidSet, std::vector<GameObject*>& goSet, std::vector<std::pair<uint64_t, uint64_t>>& hierarchy);
    static void LinkHierarchy(Scene& scene, std::vector<uint64_t>& uidSet, std::vector<GameObject*>& goSet, const std::vector<std::pair<uint64_t, uint64_t>>& hierarchy);

    static void FixReferences(Scene& scene);
    static void ResolveDefaultCamera(Scene& scene, const rapidjson::Value& json);
#pragma endregion
};