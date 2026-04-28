#pragma once
#include <memory>
#include <string>
#include <filesystem>

class Scene;
class GameObject;

struct PrefabEditSession
{
    bool                         m_active = false;
    bool                         m_editingInMainScene = false;
    Scene* m_isolatedScene;
    GameObject* m_rootObject = nullptr;

    void clear()
    {
        m_active = false;
        m_rootObject = nullptr;
        m_isolatedScene = nullptr;
    }
};