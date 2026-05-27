#pragma once

#include "ComponentType.h"
#include "PrefabInstance.h"
#include <filesystem>

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

private:
    ModuleAssets* m_moduleAssets;
};
