#pragma once
#include "ICommand.h"

#include <cstdint>

class Scene;
class GameObject;
using UID = uint64_t;

class CommandReparent : public ICommand
{
public:
    CommandReparent(Scene* scene, GameObject* child, GameObject* newParent);

    void run() override;

private:
    Scene* m_scene = nullptr;
    UID m_childID = 0;
    UID m_newParentID = 0;
};
