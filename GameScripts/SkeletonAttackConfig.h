#pragma once

#include "DataContainerAPI.h"

class SkeletonAttackConfig : public DataContainer
{
    DECLARE_DATACONTAINER(SkeletonAttackConfig)

public:
    SkeletonAttackConfig() = default;
    explicit SkeletonAttackConfig(AssetReference& id)
        : DataContainer(id)
    {
    }

    // Basic Attack (from EnemyBaseAttackConfig)
    float m_basicAttackRange = 4.0f;
    float m_basicAttackDamage = 10.0f;
    float m_basicAttackWindupTime = 0.35f;
    float m_basicAttackTotalDuration = 0.8f;
    float m_basicAttackCooldown = 1.2f;

    // --- Scimitar Strike --- //
    float m_scimitarStartRange = 4.5f;
    float m_scimitarDashStopRange = 1.5f;
    float m_scimitarDashDuration = 2.0f;
    float m_scimitarDashSpeed = 10.0f;
    float m_scimitarHalfAngleDegrees = 55.0f;

    float m_attackClipDuration = 1.33f;
    float m_attackHitTime = 0.45f;
    float m_attackAnimationSpeed = 1.3f;

    float m_stepBackDuration = 0.15f;
    float m_stepBackSpeed = 4.0f;

    float m_scimitarStunHitRange = 1.5f;
    float m_scimitarStunDuration = 3.0f;

    // --- Guard --- //
    float m_guardDuration = 5.0f;
    float m_guardCooldown = 10.0f;
    float m_guardRange = 3.5f;
    float m_guardHealPerSecond = 2.5f;
    float m_guardBlockHalfAngleDegrees = 60.0f;

    // Revive
    float m_reviveDuration = 4.0f;
    float m_downedHP = 40.0f;

    IMPLEMENT_DATACONTAINER_FIELDS(SkeletonAttackConfig,
        FIELD_GROUP_COLLAPSE("Basic Attack",
            SERIALIZED_FLOAT(m_basicAttackRange, "Basic Attack Range", 0.0f, 100.0f, 0.1f),
            SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 9999.0f, 1.0f),
            SERIALIZED_FLOAT(m_basicAttackWindupTime, "Basic Attack Windup Time", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackTotalDuration, "Basic Attack Total Duration", 0.1f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackCooldown, "Basic Attack Cooldown", 0.0f, 10.0f, 0.05f)
        ),
        FIELD_GROUP_COLLAPSE("Scimitar Strike",
            SERIALIZED_FLOAT(m_scimitarStartRange, "Scimitar Start Range", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_scimitarDashStopRange, "Scimitar Dash Stop Range", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_scimitarDashDuration, "Scimitar Dash Duration", 0.0f, 5.0f, 0.1f),
            SERIALIZED_FLOAT(m_scimitarDashSpeed, "Scimitar Dash Speed", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_attackClipDuration, "Attack Clip Duration", 0.0f, 5.0f, 0.05f),
            SERIALIZED_FLOAT(m_attackHitTime, "Attack Hit Time", 0.0f, 5.0f, 0.05f),
            SERIALIZED_FLOAT(m_attackAnimationSpeed, "Attack Animation Speed", 0.0f, 5.0f, 0.05f),
            SERIALIZED_FLOAT(m_stepBackDuration, "Step Back Duration", 0.0f, 5.0f, 0.1f),
            SERIALIZED_FLOAT(m_stepBackSpeed, "Step Back Speed", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_scimitarStunHitRange, "Scimitar Stun Hit Range", 0.0f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_scimitarStunDuration, "Scimitar Stun Duration", 0.0f, 5.0f, 0.1f),
            SERIALIZED_FLOAT(m_scimitarHalfAngleDegrees, "Scimitar Half Angle Degrees", 0.0f, 18.0f, 1.0f)
        ),
        FIELD_GROUP_COLLAPSE("Guard",
            SERIALIZED_FLOAT(m_guardDuration, "Guard Duration", 0.0f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_guardCooldown, "Guard Cooldown", 0.0f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_guardRange, "Guard Range", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_guardHealPerSecond, "Guard Heal Per Second", 0.0f, 100.0f, 1.0f),
            SERIALIZED_FLOAT(m_guardBlockHalfAngleDegrees, "Guard Half Angle Degrees", 0.0f, 180.0f, 1.0f)
        ),
        FIELD_GROUP_COLLAPSE("Revive",
            SERIALIZED_FLOAT(m_reviveDuration, "Revive Duration", 0.0f, 20.0f, 0.01f),
            SERIALIZED_FLOAT(m_downedHP, "Downed HP", 1.0f, 150.0f, 1.0f)
        )
    )
};
