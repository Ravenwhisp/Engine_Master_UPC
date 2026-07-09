#pragma once

#include "EnemyBaseAttackConfig.h"

class ArcherAttackConfig : public EnemyBaseAttackConfig
{
    DECLARE_SCRIPT(ArcherAttackConfig)

public:
    explicit ArcherAttackConfig(GameObject* owner);

    FieldList getExposedFields() const override;

public:
    // Arrow Barrage
    float m_arrowBarrageRange = 7.0f;
    float m_arrowBarrageDamage = 15.0f;
    float m_arrowBarrageRadius = 2.5f;
    float m_arrowBarrageThrowTime = 0.35f;
    float m_arrowBarrageLandDelay = 1.0f;
    float m_arrowBarrageTotalDuration = 1.7f;
    float m_arrowBarrageCooldown = 5.0f;

    // Somersault
    float m_somersaultTriggerRange = 2.0f;
    float m_somersaultDistance = 4.0f;
    float m_somersaultDuration = 0.35f;
    float m_somersaultCooldown = 9.0f;

};