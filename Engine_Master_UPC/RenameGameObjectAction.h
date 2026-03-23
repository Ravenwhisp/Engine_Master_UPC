#pragma once

class Scene;
class GameObject;
using UID = uint64_t;

class RenameGameObjectAction
{
public:
    RenameGameObjectAction(Scene* scene, GameObject* target, const std::string& newName);

    void run();

private:
    Scene* m_scene = nullptr;
    UID         m_targetID = 0;
    std::string m_newName;
};