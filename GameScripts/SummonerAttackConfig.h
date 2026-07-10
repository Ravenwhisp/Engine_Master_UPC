#pragma once

#include "DataContainerAPI.h"

class Prefab;

class SummonerAttackConfig : public DataContainer
{
    DECLARE_DATACONTAINER(SummonerAttackConfig)

public:
    SummonerAttackConfig() = default;
    explicit SummonerAttackConfig(AssetReference& id)
        : DataContainer(id)
    {
    }

    // Basic Attack (from EnemyBaseAttackConfig)
    float m_basicAttackRange = 4.0f;
    float m_basicAttackDamage = 10.0f;
    float m_basicAttackWindupTime = 0.35f;
    float m_basicAttackTotalDuration = 0.8f;
    float m_basicAttackCooldown = 1.2f;

    // Teleport
    float m_teleportCooldown = 12.0f;
    float m_teleportRadius = 6.0f;
    float m_teleportMinPlayerDistance = 5.0f;

    // Energy Ball
    float m_energyBallSpeed = 6.0f;
    float m_energyBallLifetime = 2.0f;

    // Summon
    AssetRef<Prefab> m_spiderPrefab;
    int m_summonCount = 3;
    float m_summonRadius = 2.0f;
    float m_summonCastTime = 0.5f;
    float m_summonTotalDuration = 1.0f;
    float m_summonCooldown = 8.0f;
    float m_summonRecoverDuration = 3.0f;

    IMPLEMENT_DATACONTAINER_FIELDS(SummonerAttackConfig,
        FIELD_GROUP_COLLAPSE("Basic Attack",
            SERIALIZED_FLOAT(m_basicAttackRange, "Basic Attack Range", 0.0f, 100.0f, 0.1f),
            SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 9999.0f, 1.0f),
            SERIALIZED_FLOAT(m_basicAttackWindupTime, "Basic Attack Windup Time", 0.0f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackTotalDuration, "Basic Attack Total Duration", 0.1f, 10.0f, 0.05f),
            SERIALIZED_FLOAT(m_basicAttackCooldown, "Basic Attack Cooldown", 0.0f, 10.0f, 0.05f)
        ),
        FIELD_GROUP_COLLAPSE("Teleport",
            SERIALIZED_FLOAT(m_teleportCooldown, "Teleport Cooldown", 0.0f, 30.0f, 0.1f),
            SERIALIZED_FLOAT(m_teleportRadius, "Teleport Radius", 0.0f, 30.0f, 0.1f),
            SERIALIZED_FLOAT(m_teleportMinPlayerDistance, "Min Player Distance", 0.0f, 20.0f, 0.1f)
        ),
        FIELD_GROUP_COLLAPSE("Energy Ball",
            SERIALIZED_FLOAT(m_energyBallSpeed, "Energy Ball Speed", 0.0f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_energyBallLifetime, "Energy Ball Lifetime", 0.1f, 10.0f, 0.1f)
        ),
        FIELD_GROUP_COLLAPSE("Summon",
            SERIALIZED_INT(m_summonCount, "Summon Count"),
            SERIALIZED_FLOAT(m_summonRadius, "Summon Radius", 0.0f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_summonCastTime, "Summon Cast Time", 0.0f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_summonTotalDuration, "Summon Total Duration", 1.0f, 10.0f, 0.1f),
            SERIALIZED_FLOAT(m_summonCooldown, "Summon Cooldown", 0.0f, 30.0f, 0.1f),
            SERIALIZED_FLOAT(m_summonRecoverDuration, "Summon Recover Duration", 0.0f, 10.0f, 0.1f)
        )
    )
};
