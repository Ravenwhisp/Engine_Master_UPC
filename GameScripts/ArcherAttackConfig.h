#pragma once

#include "ScriptAPI.h"

class ArcherAttackConfig : public Script
{
    DECLARE_SCRIPT(ArcherAttackConfig)

public:
    explicit ArcherAttackConfig(GameObject* owner);

    void Start() override;

    FieldList getExposedFields() const override;

public:
    // Basic attack
    float m_basicAttackRange = 4.0f;
    float m_basicAttackDamage = 10.0f;
    float m_basicAttackWindupTime = 0.35f;
    float m_basicAttackTotalDuration = 0.8f;
    float m_basicAttackCooldown = 1.2f;

    // Arrow Barrage
    float m_arrowBarrageRange = 7.0f;
    float m_arrowBarrageDamage = 15.0f;
    float m_arrowBarrageRadius = 2.5f;
    float m_arrowBarrageThrowTime = 0.35f;
    float m_arrowBarrageLandDelay = 1.0f;
    float m_arrowBarrageTotalDuration = 1.7f;
    float m_arrowBarrageCooldown = 5.0f;

    Transform* m_barrageUITransform = nullptr;
    Transform2D* m_barrageUITransform2D = nullptr;
    Transform2D* m_barrageUIGlow = nullptr;

    // Somersault
    float m_somersaultTriggerRange = 2.0f;
    float m_somersaultDistance = 4.0f;
    float m_somersaultDuration = 0.35f;
    float m_somersaultCooldown = 9.0f;

};