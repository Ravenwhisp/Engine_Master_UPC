#include "Globals.h"
#include "ComponentFactory.h"

#include "Component.h"
#include "GameObject.h"

// Normal components
#include "MeshRenderer.h"
#include "LightComponent.h"
#include "CameraComponent.h"
#include "NavigationAgentComponent.h"
#include "WaypointPathComponent.h"
#include "ScriptComponent.h"
#include "SpriteRenderer.h"

// UI components
#include "Canvas.h"
#include "Transform2D.h"
#include "UIImage.h"
#include "UIText.h"
#include "UIButton.h"

// Fake / behaviour components
#include "ChangeScene.h"
#include "ExitApplication.h"

std::unique_ptr<Component> ComponentFactory::create(ComponentType type, GameObject* owner)
{
    return createWithUID(type, GenerateUID(), owner);
}

std::unique_ptr<Component> ComponentFactory::createWithUID(ComponentType type, UID id, GameObject* owner)
{
    switch (type)
    {
    case ComponentType::MODEL:
        return std::make_unique<MeshRenderer>(id, owner);

    case ComponentType::LIGHT:
        return std::make_unique<LightComponent>(id, owner);

    case ComponentType::SCRIPT:
        return std::make_unique<ScriptComponent>(id, owner);

    case ComponentType::CAMERA:
        return std::make_unique<CameraComponent>(id, owner);

    case ComponentType::TRANSFORM2D:
        return std::make_unique<Transform2D>(id, owner);

    case ComponentType::CANVAS:
        return std::make_unique<Canvas>(id, owner);

    case ComponentType::UIIMAGE:
        return std::make_unique<UIImage>(id, owner);

    case ComponentType::UITEXT:
        return std::make_unique<UIText>(id, owner);

    case ComponentType::UIBUTTON:
        return std::make_unique<UIButton>(id, owner);

    case ComponentType::NAVIGATION_AGENT:
        return std::make_unique<NavigationAgentComponent>(id, owner);

    case ComponentType::WAYPOINT_PATH:
        return std::make_unique<WaypointPathComponent>(id, owner);

    case ComponentType::SPRITE_RENDERER:
        return std::make_unique<SpriteRenderer>(id, owner);

    case ComponentType::CHANGE_SCENE:
        return std::make_unique<ChangeScene>(id, owner);

    case ComponentType::EXIT_APPLICATION:
        return std::make_unique<ExitApplication>(id, owner);

    case ComponentType::TRANSFORM:
    case ComponentType::COUNT:
    default:
        return nullptr;
    }
}