#pragma once

class Scene;
class GameObject;
using UID = uint64_t;

class AddChildToPrefabRootAction
{
public:
    AddChildToPrefabRootAction(Scene* targetScene, GameObject* parent);

    // Returns the newly created child, or nullptr on failure.
    GameObject* run();

private:
    Scene* m_scene = nullptr;
    UID       m_parentID = 0;
};