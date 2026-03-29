#pragma once
#include "ICommand.h"

#include <cstdint>
#include <string>

class Scene;
class GameObject;
using UID = uint64_t;

class CommandAddChildToPrefabRoot : public ICommand
{
public:
    CommandAddChildToPrefabRoot(Scene* targetScene, GameObject* parent);

    void run() override;
    GameObject* getResult() const;

private:
    Scene* m_scene = nullptr;
    UID m_parentID = 0;
    GameObject* m_result = nullptr;
};
