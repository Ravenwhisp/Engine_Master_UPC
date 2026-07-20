#pragma once
#include "ICommand.h"
#include "AssetId.h"

#include <cstdint>

class Scene;
class GameObject;
using UID = uint64_t;

class CommandInstantiatePrefab : public ICommand
{
public:
    CommandInstantiatePrefab(Scene* scene, const AssetId& ref, GameObject* parent = nullptr);

    void run() override;
    GameObject* getResult() const;

private:
    Scene* m_scene = nullptr;
    AssetId m_source;
    UID m_parentID = 0;
    GameObject* m_result = nullptr;
};
