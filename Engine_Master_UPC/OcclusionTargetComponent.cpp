#include "Globals.h"
#include "OcclusionTargetComponent.h"

OcclusionTargetComponent::OcclusionTargetComponent(UID id, GameObject* owner)
    : Component(id, ComponentType::OCCLUSION_TARGET, owner)
{
}

std::unique_ptr<Component> OcclusionTargetComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<OcclusionTargetComponent> newComponent =
        std::make_unique<OcclusionTargetComponent>(m_uuid, newOwner);

    newComponent->setActive(isActive());

    return newComponent;
}