#pragma once

#include "EnemyBaseAttackConfig.h"

class SummonerAttackConfig : public EnemyBaseAttackConfig
{
	DECLARE_SCRIPT(SummonerAttackConfig)

public:
	explicit SummonerAttackConfig(GameObject* owner);

	ScriptFieldList getExposedFields() const override;

public:
	// Teleport
	float m_teleportCooldown = 12.0f;
	float m_teleportRadius = 6.0f;
	float m_teleportMinPlayerDistance = 5.0f;

	// Energy Ball
	float m_energyBallSpeed = 6.0f;
	float m_energyBallLifetime = 2.0f;

	// Summon
	std::string m_spiderPrefabPath = "";
	int m_summonCount = 3;
	float m_summonRadius = 2.0f;
	float m_summonCastTime = 0.5f;
	float m_summonTotalDuration = 1.0f;
	float m_summonCooldown = 8.0f;
	float m_summonRecoverDuration = 3.0f;
};