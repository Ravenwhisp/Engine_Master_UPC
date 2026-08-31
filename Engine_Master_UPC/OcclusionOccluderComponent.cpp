#include "Globals.h"
#include "OcclusionOccluderComponent.h"

OcclusionOccluderComponent::OcclusionOccluderComponent(UID id, GameObject* owner) : Component(id, ComponentType::OCCLUSION_OCCLUDER, owner)
{
}

std::unique_ptr<Component> OcclusionOccluderComponent::clone(GameObject* newOwner) const
{
    std::unique_ptr<OcclusionOccluderComponent> newComponent = std::make_unique<OcclusionOccluderComponent>(m_uuid, newOwner);
    newComponent->setActive(isActive());
    return newComponent;
}