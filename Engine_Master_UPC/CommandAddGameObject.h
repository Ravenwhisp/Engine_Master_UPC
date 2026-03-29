#pragma once
#include "ICommand.h"

#include <cstdint>
#include <string>

class Scene;
class GameObject;
using UID = uint64_t;

class CommandAddGameObject : public ICommand
{
public:
    explicit CommandAddGameObject(Scene* scene, GameObject* parent = nullptr);

    void run() override;
    GameObject* getResult() const;

private:
    Scene* m_scene = nullptr;
    UID m_parentID = 0;
    GameObject* m_result = nullptr;
};
