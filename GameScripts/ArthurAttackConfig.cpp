#include "pch.h"
#include "ArthurAttackConfig.h"

IMPLEMENT_SCRIPT_FIELDS(ArthurAttackConfig,
    // Heavy Swipe
    SERIALIZED_FLOAT(m_heavySwipeDamage, "Heavy Swipe Damage", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_heavySwipeRange, "Heavy Swipe Range", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_heavySwipeHalfAngleDegrees, "Heavy Swipe Half Angle Degrees", 0.0f, 180.0f, 1.0f),
    SERIALIZED_FLOAT(m_heavySwipeTotalDuration, "Heavy Swipe Total Duration", 0.1f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipeHit1Time, "Heavy Swipe Hit 1 Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipeHit2Time, "Heavy Swipe Hit 2 Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipeHit3Time, "Heavy Swipe Hit 3 Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipeRecoveryDuration, "Heavy Swipe Recovery Duration", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipePhase2Hit1Time, "Heavy Swipe Phase 2 Hit 1 Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipePhase2Hit2Time, "Heavy Swipe Phase 2 Hit 2 Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipePhase2Hit3Time, "Heavy Swipe Phase 2 Hit 3 Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipePhase2Hit4Time, "Heavy Swipe Phase 2 Hit 4 Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipePhase2RecoveryDuration, "Heavy Swipe Phase 2 Recovery Duration", 0.0f, 10.0f, 0.05f),
    // Side Sweep
    SERIALIZED_FLOAT(m_sideSweepDamage, "Side Sweep Damage", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_sideSweepRange, "Side Sweep Range", 0.0f, 30.0f, 0.1f),
    SERIALIZED_FLOAT(m_sideSweepHalfAngleDegrees, "Side Sweep Half Angle Degrees", 0.0f, 180.0f, 1.0f),
    SERIALIZED_FLOAT(m_sideSweepHitTime, "Side Sweep Hit Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_sideSweepTotalDuration, "Side Sweep Total Duration", 0.1f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_sideSweepRecoveryDuration, "Side Sweep Recovery Duration", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_sideSweepCooldown, "Side Sweep Cooldown", 0.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_sideSweepPhase2HitTime, "Side Sweep Phase 2 Hit Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_sideSweepPhase2TotalDuration, "Side Sweep Phase 2 Total Duration", 0.1f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_sideSweepPhase2RecoveryDuration, "Side Sweep Phase 2 Recovery Duration", 0.0f, 10.0f, 0.05f),
    // Charging Slam
    SERIALIZED_FLOAT(m_chargingSlamDashDamage, "Charging Slam Dash Damage", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_chargingSlamFinalAreaImpactDamage, "Charging Slam Final Area Impact Damage", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_chargingSlamHitTime, "Charging Slam Hit Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_chargingSlamDashSpeed, "Charging Slam Dash Speed", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(m_chargingSlamDashHitRadius, "Charging Slam Dash Hit Radius", 0.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_chargingSlamImpactRadius, "Charging Slam Impact Radius", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_chargingSlamTotalDuration, "Charging Slam Total Duration", 0.1f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_chargingSlamRecoveryDuration, "Charging Slam Recovery Duration", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_chargingSlamImpactStunDuration, "Charging Slam Impact Stun Duration", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_chargingSlamCooldown, "Charging Slam Cooldown", 0.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_chargingSlamPhase2HitTime, "Charging Slam Phase 2 Hit Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_chargingSlamPhase2DashSpeed, "Charging Slam Phase 2 Dash Speed", 0.0f, 50.0f, 0.1f),
    SERIALIZED_FLOAT(m_chargingSlamMinRange, "Charging Slam Min Range", 0.0f, 20.0f, 0.1f),
    SERIALIZED_FLOAT(m_chargingSlamMaxRange, "Charging Slam Max Range", 0.0f, 20.0f, 0.1f),
    // Earth Hammer
    SERIALIZED_FLOAT(m_earthHammerDamage, "Earth Hammer Damage", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_earthHammerRadius, "Earth Hammer Radius", 0.0f, 30.0f, 0.1f),
    SERIALIZED_FLOAT(m_earthHammerTotalDuration, "Earth Hammer Total Duration", 0.1f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_earthHammerHitTime, "Earth Hammer Hit Time", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_earthHammerRecoveryDuration, "Earth Hammer Recovery Duration", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_earthHammerStunDuration, "Earth Hammer Stun Duration", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_earthHammerCooldown, "Earth Hammer Cooldown", 0.0f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_earthHammerPhase2Damage, "Earth Hammer Phase 2 Damage", 0.0f, 9999.0f, 1.0f),
    SERIALIZED_FLOAT(m_earthHammerPhase2StunDuration, "Earth Hammer Phase 2 Stun Duration", 0.0f, 10.0f, 0.05f)
)

ArthurAttackConfig::ArthurAttackConfig(GameObject* owner)
    : Script(owner)
{
}

IMPLEMENT_SCRIPT(ArthurAttackConfig)