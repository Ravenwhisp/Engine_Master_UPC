#pragma once

#include "ScriptAPI.h"

class ArthurAttackConfig : public Script
{
    DECLARE_SCRIPT(ArthurAttackConfig)

public:
    explicit ArthurAttackConfig(GameObject* owner);

    ScriptFieldList getExposedFields() const override;

public:
    // Heavy Swipe
    float m_heavySwipeDamage = 10.0f;
    float m_heavySwipeRange = 3.0f;
    float m_heavySwipeHalfAngleDegrees = 60.0f;

    float m_heavySwipeTotalDuration = 1.5f;
    float m_heavySwipeHit1Time = 0.45f;
    float m_heavySwipeHit2Time = 0.95f;
    float m_heavySwipeHit3Time = 1.35f;

    float m_heavySwipeRecoveryDuration = 0.75f;

    float m_heavySwipePhase2Hit1Time = 0.30f;
    float m_heavySwipePhase2Hit2Time = 0.60f;
    float m_heavySwipePhase2Hit3Time = 0.90f;
    float m_heavySwipePhase2Hit4Time = 1.20f;
    float m_heavySwipePhase2RecoveryDuration = 0.45f;

    // Side Sweep
    float m_sideSweepDamage = 12.0f;
    float m_sideSweepRange = 4.0f;
    float m_sideSweepHalfAngleDegrees = 55.0f;

    float m_sideSweepHitTime = 0.35f;
    float m_sideSweepTotalDuration = 0.7f;

    float m_sideSweepRecoveryDuration = 0.5f;
    
    float m_sideSweepCooldown = 2.0f;

    float m_sideSweepPhase2HitTime = 0.20f;
    float m_sideSweepPhase2TotalDuration = 0.55f;
    float m_sideSweepPhase2RecoveryDuration = 0.35f;

    // Charging Slam
    float m_chargingSlamDashDamage = 12.0f;
    float m_chargingSlamFinalAreaImpactDamage = 25.0f;

    float m_chargingSlamHitTime = 1.0f;
    float m_chargingSlamDashSpeed = 8.0f;

    float m_chargingSlamDashHitRadius = 1.2f;
    float m_chargingSlamImpactRadius = 3.0f;

    float m_chargingSlamTotalDuration = 2.2f;
    float m_chargingSlamRecoveryDuration = 0.75f;

    float m_chargingSlamImpactStunDuration = 1.0f;
    
    float m_chargingSlamCooldown = 2.0f;

    float m_chargingSlamPhase2HitTime = 0.5f;
    float m_chargingSlamPhase2DashSpeed = 10.0f;

    float m_chargingSlamMinRange = 5.0f;
    float m_chargingSlamMaxRange = 8.0f;

    // Earth Hammer
    float m_earthHammerDamage = 20.0f;
    float m_earthHammerRadius = 5.0f;

    float m_earthHammerTotalDuration = 2.2f;
    float m_earthHammerHitTime = 2.0f;

    float m_earthHammerRecoveryDuration = 1.0f;

    float m_earthHammerStunDuration = 1.25f;
    
    float m_earthHammerCooldown = 2.0f;

    float m_earthHammerPhase2Damage = 25.0f;
    float m_earthHammerPhase2StunDuration = 1.75f;
};