#pragma once

#include "EnemyBaseAttackConfig.h"

class Prefab;

class SummonerAttackConfig : public EnemyBaseAttackConfig
{
    DECLARE_DATACONTAINER(SummonerAttackConfig)

public:
    SummonerAttackConfig() = default;
    explicit SummonerAttackConfig(AssetId& id)
        : EnemyBaseAttackConfig(id)
    {
    }

    // Teleport
    float m_teleportCooldown = 12.0f;
    float m_teleportRadius = 6.0f;
    float m_teleportMinPlayerDistance = 5.0f;

    // Energy Ball
    float m_energyBallSpeed = 6.0f;
    float m_energyBallLifetime = 2.0f;

    // Summon
    AssetReference<Prefab> m_spiderPrefab;
    int m_summonCount = 3;
    float m_summonRadius = 2.0f;
    float m_summonCastTime = 0.5f;
    float m_summonTotalDuration = 1.0f;
    float m_summonCooldown = 8.0f;
    float m_summonRecoverDuration = 3.0f;

    IMPLEMENT_DATACONTAINER_FIELDS_INHERITED(SummonerAttackConfig, EnemyBaseAttackConfig,
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
