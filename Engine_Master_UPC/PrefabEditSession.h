#pragma once
#include "SceneModule.h"
#include <string>
#include <memory>

class GameObject;

struct PrefabEditSession
{
    bool                          active = false;
    std::string                   prefabName;
    std::unique_ptr<SceneModule>  isolatedScene;
    GameObject* rootObject = nullptr;

    void clear()
    {
        active = false;
        prefabName.clear();
        rootObject = nullptr;
        isolatedScene.reset();
    }
};