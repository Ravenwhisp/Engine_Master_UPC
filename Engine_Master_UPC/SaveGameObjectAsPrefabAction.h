#pragma once
#include <filesystem>

class GameObject;

class SaveGameObjectAsPrefabAction
{
public:
    SaveGameObjectAsPrefabAction(GameObject* go, const std::filesystem::path& targetDir);

    void run();

private:
    GameObject* m_go = nullptr;
    std::filesystem::path m_targetDir;
};