#pragma once

#include <memory>

#include "ComponentType.h"
#include "UID.h"

class Component;
class GameObject;

class ComponentFactory
{
public:
    static std::unique_ptr<Component> create(ComponentType type, GameObject* owner);
    static std::unique_ptr<Component> createWithUID(ComponentType type, UID id, GameObject* owner);
};