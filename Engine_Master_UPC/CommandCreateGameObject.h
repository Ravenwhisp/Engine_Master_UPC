#pragma once
#include "ICommand.h"

#include <cstdint>

class Scene;
class GameObject;
using UID = uint64_t;

enum class ComponentType;

class CommandCreateGameObject : public ICommand
{
public:
    CommandCreateGameObject(Scene* scene, GameObject* parent, ComponentType componentType);

    void run() override;
    GameObject* getResult() const;

private:
    Scene* m_scene = nullptr;
    UID m_parentID = 0;
    ComponentType m_componentType;
    GameObject* m_result = nullptr;
};
