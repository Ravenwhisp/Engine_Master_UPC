#pragma once

#include "Damageable.h"

class BreakableObject;

class BreakableDamageable : public Damageable
{
    DECLARE_SCRIPT(BreakableDamageable)

public:
    explicit BreakableDamageable(GameObject* owner);
    void Start() override;

protected:
    void onDeath() override;

private:
    BreakableObject* m_breakableObject = nullptr;
};