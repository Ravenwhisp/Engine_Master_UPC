#pragma once
#include "ICommand.h"

#include <cstdint>
#include <filesystem>

class Scene;
class GameObject;
using UID = uint64_t;

class CommandInstantiatePrefab : public ICommand
{
public:
    CommandInstantiatePrefab(Scene* scene, const std::filesystem::path& sourcePath, GameObject* parent = nullptr);

    void run() override;
    GameObject* getResult() const;

private:
    Scene* m_scene = nullptr;
    std::filesystem::path m_source;
    UID m_parentID = 0;
    GameObject* m_result = nullptr;
};
