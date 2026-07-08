#pragma once

#include "ComponentType.h"
#include "PrefabInstance.h"
#include "AssetReference.h"
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

    bool applyPrefab(const GameObject* go);
    bool revertPrefab(GameObject* go, Scene* scene);
    bool createVariant(const std::filesystem::path& src, const std::filesystem::path& dst);
    GameObject* spawnPrefab(const Prefab& prefab, Scene* scene);
    GameObject* spawnPrefab(const AssetReference& ref, Scene* scene);

private:
    ModuleAssets* m_moduleAssets;
};
