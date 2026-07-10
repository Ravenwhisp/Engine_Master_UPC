#pragma once

#include "DataContainerAPI.h"

class PaladinAttackConfig : public DataContainer
{
    DECLARE_DATACONTAINER(PaladinAttackConfig)

public:
    PaladinAttackConfig() = default;
    explicit PaladinAttackConfig(AssetReference& id)
        : DataContainer(id)
    {
    }

    // Basic Attack (from EnemyBaseAttackConfig)
    float m_basicAttackRange = 4.0f;
    float m_basicAttackDamage = 10.0f;
    float m_basicAttackWindupTime = 0.35f;
    float m_basicAttackTotalDuration = 0.8f;
    float m_basicAttackCooldown = 1.2f;

    // Charge
    float m_chargeRange = 5.0f;
    float m_chargeDuration = 0.5f;
    float m_chargeSpeed = 10.0f;
    float m_chargeCooldown = 3.0f;

    IMPLEMENT_DATACONTAINER_FIELDS(PaladinAttackConfig,
        FIELD_GROUP_COLLAPSE("Basic Attack",
            SERIALIZED_FLOAT(m_basicAttackRange, "Basic Attack Range", 0.0f, 100.0f, 0.1f),
            SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 9999.0f, 1.0f),
            SERIALIZED_FLOAT(m_basicAttackWindupTime, "Basic Attack Windup Time", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackTotalDuration, "Basic Attack Total Duration", 0.1f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackCooldown, "Basic Attack Cooldown", 0.0f, 10.0f, 0.05f)
        ),
        FIELD_GROUP_COLLAPSE("Charge",
            SERIALIZED_FLOAT(m_chargeRange, "Charge Range", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_chargeDuration, "Charge Duration", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_chargeSpeed, "Charge Speed", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_chargeCooldown, "Charge Cooldown", 0.0f, 10.0f, 0.05f)
        )
    )
};
