#include "Globals.h"
#include "ComponentFactory.h"

#include "Component.h"
#include "GameObject.h"

// Normal components
#include "MeshRenderer.h"
#include "LightComponent.h"
#include "CameraComponent.h"
#include "NavModifierVolumeComponent.h"
#include "NavRuntimeBlockerComponent.h"
#include "ScriptComponent.h"
#include "AnimationComponent.h"
#include "TriggerComponent.h"
#include "ParticleSystemComponent.h"
#include "TrailComponent.h"
#include "ComponentSoundListener.h"
#include "ComponentSoundSource.h"
#include "PlayerRenderBufferComponent.h"
#include "EnemyRenderBufferComponent.h"

// Prefab
#include "PrefabInstanceComponent.h"

// UI components
#include "Canvas.h"
#include "Transform2D.h"
#include "UIImage.h"
#include "UIText.h"
#include "UIButton.h"
#include "UISlider.h"
#include "UISheet.h"


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
    
    case ComponentType::NAV_MODIFIER_VOLUME:
        return std::make_unique<NavModifierVolumeComponent>(id, owner);

    case ComponentType::NAV_RUNTIME_BLOCKER:
        return std::make_unique<NavRuntimeBlockerComponent>(id, owner);

    case ComponentType::ANIMATION:
        return std::make_unique<AnimationComponent>(id, owner);

    case ComponentType::UISLIDER:
		return std::make_unique<UISlider>(id, owner);

    case ComponentType::UISHEET:
        return std::make_unique<UISheet>(id, owner);

    case ComponentType::TRIGGER:
        return std::make_unique<TriggerComponent>(id, owner);

    case ComponentType::PARTICLE_SYSTEM:
        return std::make_unique<ParticleSystemComponent>(id, owner);

    case ComponentType::TRAIL:
        return std::make_unique<TrailComponent>(id, owner);

    case ComponentType::SOUND_LISTENER:
        return std::make_unique<ComponentSoundListener>(id, owner);

    case ComponentType::SOUND_SOURCE:
        return std::make_unique<ComponentSoundSource>(id, owner);

    case ComponentType::PREFAB_INSTANCE:
        return std::make_unique<PrefabInstanceComponent>(id, owner);

    case ComponentType::PLAYER_RENDER_BUFFER:
        return std::make_unique<PlayerRenderBufferComponent>(id, owner);

    case ComponentType::ENEMY_RENDER_BUFFER:
        return std::make_unique<EnemyRenderBufferComponent>(id, owner);

    case ComponentType::TRANSFORM:
    case ComponentType::COUNT:
    default:
        return nullptr;
    }
}