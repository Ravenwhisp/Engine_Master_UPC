#pragma once
#include "ICommand.h"

#include <cstdint>

class Scene;
class GameObject;
using UID = uint64_t;

class CommandRemoveGameObject : public ICommand
{
public:
    CommandRemoveGameObject(Scene* scene, GameObject* target);

    void run() override;

private:
    Scene* m_scene = nullptr;
    UID m_targetID = 0;
};
