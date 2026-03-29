#pragma once
#include "ICommand.h"

#include <cstdint>
#include <string>

class Scene;
class GameObject;
using UID = uint64_t;

class CommandRenameGameObject : public ICommand
{
public:
    CommandRenameGameObject(Scene* scene, GameObject* target, const std::string& newName);

    void run() override;

private:
    Scene* m_scene = nullptr;
    UID m_targetID = 0;
    std::string m_newName;
};
