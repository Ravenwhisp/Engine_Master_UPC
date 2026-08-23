#pragma once
#include "Component.h"

class OcclusionTargetComponent : public Component
{
public:
    OcclusionTargetComponent(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;
};

