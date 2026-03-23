#pragma once

class Scene;
class GameObject;
using UID = uint64_t;

class ReparentAction
{
public:

    ReparentAction(Scene* scene, GameObject* child, GameObject* newParent);

    void run();

private:
    Scene* m_scene = nullptr;
    UID       m_childID = 0;
    UID       m_newParentID = 0;
};