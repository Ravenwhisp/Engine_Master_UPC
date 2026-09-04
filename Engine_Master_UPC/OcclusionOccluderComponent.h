#pragma once
#include "Component.h"

class OcclusionOccluderComponent : public Component
{
public:
    OcclusionOccluderComponent(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;
};