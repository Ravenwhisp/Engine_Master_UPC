#pragma once

#include "Component.h"

class Script : public Component
{
public:

    Script(UID id, GameObject* owner)
        : Component(id, ComponentType::SCRIPT, owner)
    {
    }

    virtual ~Script() = default;

    virtual void Start() {}

    virtual void Update() {}

    // Component update calls script update
    void update() override
    {
        Update();
    }

    std::unique_ptr<Component> clone(GameObject* newOwner) const override
    {
        return nullptr;
    }
};