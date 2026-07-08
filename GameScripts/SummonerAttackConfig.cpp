#include "pch.h"
#include "SummonerAttackConfig.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(SummonerAttackConfig, EnemyBaseAttackConfig,
	// Teleport
	FIELD_GROUP_COLLAPSE("Teleport",
		SERIALIZED_FLOAT(m_teleportCooldown, "Teleport Cooldown", 0.0f, 30.0f, 0.1f),
		SERIALIZED_FLOAT(m_teleportRadius, "Teleport Radius", 0.0f, 30.0f, 0.1f),
		SERIALIZED_FLOAT(m_teleportMinPlayerDistance, "Min Player Distance", 0.0f, 20.0f, 0.1f)
	),
	// Energy Ball
	FIELD_GROUP_COLLAPSE("Energy Ball",
		SERIALIZED_FLOAT(m_energyBallSpeed, "Energy Ball Speed", 0.0f, 10.0f, 0.1f),
		SERIALIZED_FLOAT(m_energyBallLifetime, "Energy Ball Lifetime", 0.1f, 10.0f, 0.1f)
	),
	// Summon
	FIELD_GROUP_COLLAPSE("Summon",
		SERIALIZED_STRING(m_spiderPrefabPath, "Spider Prefab Path"),
		SERIALIZED_INT(m_summonCount, "Summon Count"),
		SERIALIZED_FLOAT(m_summonRadius, "Summon Radius", 0.0f, 10.0f, 0.1f),
		SERIALIZED_FLOAT(m_summonCastTime, "Summon Cast Time", 0.0f, 10.0f, 0.1f),
		SERIALIZED_FLOAT(m_summonTotalDuration, "Summon Total Duration", 1.0f, 10.0f, 0.1f),
		SERIALIZED_FLOAT(m_summonCooldown, "Summon Cooldown", 0.0f, 30.0f, 0.1f),
		SERIALIZED_FLOAT(m_summonRecoverDuration, "Summon Recover Duration", 0.0f, 10.0f, 0.1f)
	)
)

SummonerAttackConfig::SummonerAttackConfig(GameObject* owner)
	: EnemyBaseAttackConfig(owner)
{
}

IMPLEMENT_SCRIPT(SummonerAttackConfig)