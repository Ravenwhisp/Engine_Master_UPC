#pragma once
#include <filesystem>

class Scene;
class GameObject;
using UID = uint64_t;

class InstantiatePrefabAction
{
public:
    // parent == nullptr → placed at scene root
    InstantiatePrefabAction(Scene* scene,  const std::filesystem::path& sourcePath, GameObject* parent = nullptr);

    // Returns the spawned root object, or nullptr on failure.
    GameObject* run();

private:
    Scene* m_scene = nullptr;
    std::filesystem::path m_source;
    UID                   m_parentID = 0;
};