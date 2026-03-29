#pragma once
#include "ICommand.h"

#include <filesystem>

class GameObject;

class CommandSaveGameObjectAsPrefab : public ICommand
{
public:
    CommandSaveGameObjectAsPrefab(GameObject* go, const std::filesystem::path& targetDir);

    void run() override;

private:
    GameObject* m_go = nullptr;
    std::filesystem::path m_targetDir;
};
