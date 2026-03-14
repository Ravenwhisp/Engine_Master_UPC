#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "UID.h"

class Component;
class GameObject;
class CameraComponent;

struct SceneSnapshot
{
    std::unordered_map<UID, Component*> componentMap;
    // std::unordered_map<GameObject*, GameObject*> gameObjectMap;

    std::vector<std::unique_ptr<GameObject>> allObjects;
    std::vector<GameObject*> rootObjects;
    CameraComponent* defaultCamera = nullptr;
};