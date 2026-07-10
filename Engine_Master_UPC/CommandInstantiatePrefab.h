#pragma once
#include "ICommand.h"
#include "AssetReference.h"

#include <cstdint>

class Scene;
class GameObject;
using UID = uint64_t;

class CommandInstantiatePrefab : public ICommand
{
public:
    CommandInstantiatePrefab(Scene* scene, const AssetReference& ref, GameObject* parent = nullptr);

    void run() override;
    GameObject* getResult() const;

private:
    Scene* m_scene = nullptr;
    AssetReference m_source;
    UID m_parentID = 0;
    GameObject* m_result = nullptr;
};
