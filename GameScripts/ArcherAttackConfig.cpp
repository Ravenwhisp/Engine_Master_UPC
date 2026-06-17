#include "pch.h"
#include "ArcherAttackConfig.h"

#include "Transform2D.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(ArcherAttackConfig, EnemyBaseAttackConfig,
    // Arrow Barrage
    FIELD_GROUP_COLLAPSE("Arrow Barrage",
        SERIALIZED_FLOAT(m_arrowBarrageRange, "Arrow Barrage Range", 0.0f, 9999.0f, 1.0f),
        SERIALIZED_FLOAT(m_arrowBarrageDamage, "Arrow Barrage Damage", 0.0f, 9999.0f, 1.0f),
        SERIALIZED_FLOAT(m_arrowBarrageRadius, "Arrow Barrage Radius", 0.0f, 20.0f, 0.1f),
        SERIALIZED_FLOAT(m_arrowBarrageThrowTime, "Arrow Barrage Throw Time", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_arrowBarrageLandDelay, "Arrow Barrage Land Delay", 0.0f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_arrowBarrageTotalDuration, "Arrow Barrage Total Duration", 0.1f, 10.0f, 0.05f),
        SERIALIZED_FLOAT(m_arrowBarrageCooldown, "Arrow Barrage Cooldown", 0.0f, 30.0f, 0.1f)
    ),
    // Somersault
    FIELD_GROUP_COLLAPSE("Somersault",
        SERIALIZED_FLOAT(m_somersaultTriggerRange, "Somersault Trigger Range", 0.0f, 20.0f, 0.1f),
        SERIALIZED_FLOAT(m_somersaultDistance, "Somersault Distance", 0.0f, 20.0f, 0.1f),
        SERIALIZED_FLOAT(m_somersaultDuration, "Somersault Duration", 0.0f, 5.0f, 0.05f),
        SERIALIZED_FLOAT(m_somersaultCooldown, "Somersault Cooldown", 0.0f, 30.0f, 0.1f)
    )
)

ArcherAttackConfig::ArcherAttackConfig(GameObject* owner)
    : EnemyBaseAttackConfig(owner)
{
}

IMPLEMENT_SCRIPT(ArcherAttackConfig)