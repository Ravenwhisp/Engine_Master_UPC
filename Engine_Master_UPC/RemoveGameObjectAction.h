#pragma once

class Scene;
class GameObject;
using UID = uint64_t;

class RemoveGameObjectAction
{
public:
    RemoveGameObjectAction(Scene* scene, GameObject* target);

    void run();

private:
    Scene* m_scene = nullptr;
    UID    m_targetID = 0;
};