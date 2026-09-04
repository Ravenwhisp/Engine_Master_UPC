#pragma once

#include "EnemyBaseAttackConfig.h"

class ArcherAttackConfig : public EnemyBaseAttackConfig
{
    DECLARE_DATACONTAINER(ArcherAttackConfig)

public:
    ArcherAttackConfig() = default;
    explicit ArcherAttackConfig(AssetId& id)
        : EnemyBaseAttackConfig(id)
    {
    }

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

    IMPLEMENT_DATACONTAINER_FIELDS_INHERITED(ArcherAttackConfig, EnemyBaseAttackConfig,
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
