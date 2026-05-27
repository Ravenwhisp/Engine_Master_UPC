#pragma once

#include "ComponentType.h"
#include "PrefabInstance.h"
#include <filesystem>
#include <rapidjson/document.h>

class ModuleAssets;
class Prefab;
class Scene;
class GameObject;

class PrefabManager
{
public:
	PrefabManager(ModuleAssets* moduleAssets);
	~PrefabManager();

    bool savePrefab(GameObject* go, const std::filesystem::path& savePath);
    bool applyPrefab(const GameObject* go);
    bool revertPrefab(GameObject* go, Scene* scene);
    bool createVariant(const std::filesystem::path& src, const std::filesystem::path& dst);
    GameObject* spawnPrefab(const Prefab& prefab, Scene* scene);
    GameObject* spawnPrefab(const std::filesystem::path& sourcePath, Scene* scene);

    // Recursively creates a GameObject tree from a JSON node (used by scene and prefab loading)
    static GameObject* createFromJSON(const rapidjson::Value& node, Scene* scene, GameObject* parent);
    static GameObject* createFromJSON(const rapidjson::Value& node, GameObject* parent);

private:
    ModuleAssets* m_moduleAssets;
};
