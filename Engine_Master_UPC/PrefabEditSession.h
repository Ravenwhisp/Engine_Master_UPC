#pragma once
#include "ModuleScene.h"
#include <memory>
#include <string>

class GameObject;

struct PrefabEditSession
{
    bool                         m_active = false;
    bool                         m_editingInMainScene = false;
    std::filesystem::path        m_sourcePath;           // full path — replaces m_prefabName
    std::unique_ptr<ModuleScene> m_isolatedScene;
    GameObject* m_rootObject = nullptr;

    void clear()
    {
        m_active = false;
        m_editingInMainScene = false;
        m_sourcePath.clear();
        m_rootObject = nullptr;
        m_isolatedScene.reset();
    }
};