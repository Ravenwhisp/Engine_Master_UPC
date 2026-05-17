#pragma once

#include <filesystem>

class ModuleAssets;
class PrefabAsset;
class Scene;
class GameObject;

class PrefabManager
{
private:
    ModuleAssets* m_moduleAssets;

public:
	PrefabManager(ModuleAssets* moduleAssets);
	~PrefabManager();

    bool savePrefab(GameObject* go, const std::filesystem::path& savePath);
    bool applyPrefab(const GameObject* go);
    bool revertPrefab(GameObject* go, Scene* scene);
    bool createVariant(const std::filesystem::path& src, const std::filesystem::path& dst);
    GameObject* spawnPrefab(const PrefabAsset& asset, Scene* scene);
    GameObject* spawnPrefab(const std::filesystem::path& sourcePath, Scene* scene);
};

