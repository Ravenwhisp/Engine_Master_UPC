#pragma once

#include "DataContainerAPI.h"
#include "Transform2D.h"
#include "UISlider.h"

class ArthurAttackConfig : public DataContainer
{
    DECLARE_DATACONTAINER(ArthurAttackConfig)

public:
    ArthurAttackConfig() = default;
    explicit ArthurAttackConfig(AssetReference& id)
        : DataContainer(id)
    {
    }

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

    IMPLEMENT_DATACONTAINER_FIELDS(ArthurAttackConfig,
        FIELD_GROUP_COLLAPSE("Heavy Swipe",
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
            SERIALIZED_FLOAT(m_heavySwipePhase2RecoveryDuration, "Heavy Swipe Phase 2 Recovery Duration", 0.0f, 10.0f, 0.05f)
        ),
        FIELD_GROUP_COLLAPSE("Side Sweep",
            SERIALIZED_FLOAT(m_sideSweepDamage, "Side Sweep Damage", 0.0f, 9999.0f, 1.0f),
            SERIALIZED_FLOAT(m_sideSweepRange, "Side Sweep Range", 0.0f, 30.0f, 0.1f),
            SERIALIZED_FLOAT(m_sideSweepHalfAngleDegrees, "Side Sweep Half Angle Degrees", 0.0f, 180.0f, 1.0f),
            SERIALIZED_FLOAT(m_sideSweepHitTime, "Side Sweep Hit Time", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_sideSweepTotalDuration, "Side Sweep Total Duration", 0.1f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_sideSweepRecoveryDuration, "Side Sweep Recovery Duration", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_sideSweepCooldown, "Side Sweep Cooldown", 0.0f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_sideSweepPhase2HitTime, "Side Sweep Phase 2 Hit Time", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_sideSweepPhase2TotalDuration, "Side Sweep Phase 2 Total Duration", 0.1f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_sideSweepPhase2RecoveryDuration, "Side Sweep Phase 2 Recovery Duration", 0.0f, 10.0f, 0.05f)
        ),
        FIELD_GROUP_COLLAPSE("Charging Slam",
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
            SERIALIZED_FLOAT(m_chargingSlamMaxRange, "Charging Slam Max Range", 0.0f, 20.0f, 0.1f)
        ),
        FIELD_GROUP_COLLAPSE("Earth Hammer",
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
    )
};
