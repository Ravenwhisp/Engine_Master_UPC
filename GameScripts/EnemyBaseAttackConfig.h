#pragma once

#include "ScriptAPI.h"

class EnemyBaseAttackConfig : public Script
{
    DECLARE_SCRIPT(EnemyBaseAttackConfig)

public:
    explicit EnemyBaseAttackConfig(GameObject* owner);

    ScriptFieldList getExposedFields() const override;

public:
    // Basic attack
    float m_basicAttackRange = 4.0f;
    float m_basicAttackDamage = 10.0f;
    float m_basicAttackWindupTime = 0.35f;
    float m_basicAttackTotalDuration = 0.8f;
    float m_basicAttackCooldown = 1.2f;
};