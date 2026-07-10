#pragma once

#include "DataContainerAPI.h"

class ArcherAttackConfig : public DataContainer
{
    DECLARE_DATACONTAINER(ArcherAttackConfig)

public:
    ArcherAttackConfig() = default;
    explicit ArcherAttackConfig(AssetReference& id)
        : DataContainer(id)
    {
    }

    // Basic Attack (from EnemyBaseAttackConfig)
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

    // Somersault
    float m_somersaultTriggerRange = 2.0f;
    float m_somersaultDistance = 4.0f;
    float m_somersaultDuration = 0.35f;
    float m_somersaultCooldown = 9.0f;

    IMPLEMENT_DATACONTAINER_FIELDS(ArcherAttackConfig,
        FIELD_GROUP_COLLAPSE("Basic Attack",
            SERIALIZED_FLOAT(m_basicAttackRange, "Basic Attack Range", 0.0f, 100.0f, 0.1f),
            SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 9999.0f, 1.0f),
            SERIALIZED_FLOAT(m_basicAttackWindupTime, "Basic Attack Windup Time", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackTotalDuration, "Basic Attack Total Duration", 0.1f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackCooldown, "Basic Attack Cooldown", 0.0f, 10.0f, 0.05f)
        ),
        FIELD_GROUP_COLLAPSE("Arrow Barrage",
            SERIALIZED_FLOAT(m_arrowBarrageRange, "Arrow Barrage Range", 0.0f, 9999.0f, 1.0f),
            SERIALIZED_FLOAT(m_arrowBarrageDamage, "Arrow Barrage Damage", 0.0f, 9999.0f, 1.0f),
            SERIALIZED_FLOAT(m_arrowBarrageRadius, "Arrow Barrage Radius", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_arrowBarrageThrowTime, "Arrow Barrage Throw Time", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_arrowBarrageLandDelay, "Arrow Barrage Land Delay", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_arrowBarrageTotalDuration, "Arrow Barrage Total Duration", 0.1f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_arrowBarrageCooldown, "Arrow Barrage Cooldown", 0.0f, 30.0f, 0.1f)
        ),
        FIELD_GROUP_COLLAPSE("Somersault",
            SERIALIZED_FLOAT(m_somersaultTriggerRange, "Somersault Trigger Range", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_somersaultDistance, "Somersault Distance", 0.0f, 20.0f, 0.1f),
            SERIALIZED_FLOAT(m_somersaultDuration, "Somersault Duration", 0.0f, 5.0f, 0.05f),
            SERIALIZED_FLOAT(m_somersaultCooldown, "Somersault Cooldown", 0.0f, 30.0f, 0.1f)
        )
    )
};
