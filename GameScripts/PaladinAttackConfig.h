#pragma once

#include "EnemyBaseAttackConfig.h"

class PaladinAttackConfig : public EnemyBaseAttackConfig
{
    DECLARE_DATACONTAINER(PaladinAttackConfig)

public:
    PaladinAttackConfig() = default;
    explicit PaladinAttackConfig(AssetId& id)
        : EnemyBaseAttackConfig(id)
    {
    }

    // Charge
    float m_chargeRange = 5.0f;
    float m_chargeDuration = 0.5f;
    float m_chargeSpeed = 10.0f;
    float m_chargeCooldown = 3.0f;

    IMPLEMENT_DATACONTAINER_FIELDS_INHERITED(PaladinAttackConfig, EnemyBaseAttackConfig,
        FIELD_GROUP_COLLAPSE("Charge",
            SERIALIZED_FLOAT(m_chargeRange, "Charge Range", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_chargeDuration, "Charge Duration", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_chargeSpeed, "Charge Speed", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_chargeCooldown, "Charge Cooldown", 0.0f, 10.0f, 0.05f)
        )
    )
};
