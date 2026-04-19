#pragma once

#include "ScriptAPI.h"

class Damageable;

class DebugDamageTrigger : public Script
{
    DECLARE_SCRIPT(DebugDamageTrigger)

public:
    explicit DebugDamageTrigger(GameObject* owner);

    void Start() override;
    void Update() override;
    ScriptFieldList getExposedFields() const override;

private:
    Damageable* findDamageable() const;

public:
    float m_damageAmount = 25.0f;
    float m_healAmount = 25.0f;

    int m_playerIndex = 0;
private:
    Damageable* m_damageable = nullptr;
};