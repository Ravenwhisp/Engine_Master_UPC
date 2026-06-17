#pragma once

#include "EnemyBaseAttackConfig.h"

class PaladinAttackConfig : public EnemyBaseAttackConfig
{
	DECLARE_SCRIPT(PaladinAttackConfig)

public:
	explicit PaladinAttackConfig(GameObject* owner);

	ScriptFieldList getExposedFields() const override;

public:
	// Charge
	float m_chargeRange = 5.0f;
	float m_chargeDuration = 0.5f;
	float m_chargeSpeed = 10.0f;
	float m_chargeCooldown = 3.0f;
};