#pragma once
class Scene;
class GameObject;
using UID = uint64_t;

class AddGameObjectAction
{
public:
    // parent == nullptr → created at scene root
    explicit AddGameObjectAction(Scene* scene, GameObject* parent = nullptr);

    GameObject* run();

private:
    Scene* m_scene = nullptr;
    UID       m_parentID = 0;
};