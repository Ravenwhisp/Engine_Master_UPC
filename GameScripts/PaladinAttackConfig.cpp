#include "pch.h"
#include "PaladinAttackConfig.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(PaladinAttackConfig, EnemyBaseAttackConfig,
	// Charge
	FIELD_GROUP_COLLAPSE("Charge",
		SERIALIZED_FLOAT(m_chargeRange, "Charge Range", 0.0f, 20.0f, 0.1f),
		SERIALIZED_FLOAT(m_chargeDuration, "Charge Duration", 0.0f, 10.0f, 0.05f),
		SERIALIZED_FLOAT(m_chargeSpeed, "Charge Speed", 0.0f, 10.0f, 0.05f),
		SERIALIZED_FLOAT(m_chargeCooldown, "Charge Cooldown", 0.0f, 10.0f, 0.05f)
	)
)

PaladinAttackConfig::PaladinAttackConfig(GameObject* owner)
	: EnemyBaseAttackConfig(owner)
{
}

IMPLEMENT_SCRIPT(PaladinAttackConfig)