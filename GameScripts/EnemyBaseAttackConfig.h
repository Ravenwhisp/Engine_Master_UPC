#pragma once

#include "DataContainerAPI.h"

class EnemyBaseAttackConfig : public DataContainer
{
    DECLARE_DATACONTAINER(EnemyBaseAttackConfig)

public:
    EnemyBaseAttackConfig() = default;
    explicit EnemyBaseAttackConfig(AssetReference& id)
        : DataContainer(id)
    {
    }

    float m_basicAttackRange = 4.0f;
    float m_basicAttackDamage = 10.0f;
    float m_basicAttackWindupTime = 0.35f;
    float m_basicAttackTotalDuration = 0.8f;
    float m_basicAttackCooldown = 1.2f;

    IMPLEMENT_DATACONTAINER_FIELDS(EnemyBaseAttackConfig,
        FIELD_GROUP_COLLAPSE("Basic Attack",
            SERIALIZED_FLOAT(m_basicAttackRange, "Basic Attack Range", 0.0f, 100.0f, 0.1f),
            SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 9999.0f, 1.0f),
            SERIALIZED_FLOAT(m_basicAttackWindupTime, "Basic Attack Windup Time", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackTotalDuration, "Basic Attack Total Duration", 0.1f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackCooldown, "Basic Attack Cooldown", 0.0f, 10.0f, 0.05f)
        )
    )
};
