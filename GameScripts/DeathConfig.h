#pragma once

#include "DataContainerAPI.h"

class DeathConfig : public DataContainer
{
	DECLARE_DATACONTAINER(DeathConfig)

public:
	DeathConfig() = default;
	explicit DeathConfig(AssetId& id)
		: DataContainer(id)
	{
	}

	// Combo
	float m_comboWindow = 1.0f;
	float m_comboWindowR2 = 2.0f;
	float m_comboWindowMaxCharge = 3.0f;
	float m_comboCooldown = 1.0f;

	// Basic Attack
	float m_basicAttackDamage = 20.0f;
	float m_basicAttackRange = 1.5f;
	float m_basicAttackHitAngle = 50.0f;
	float m_basicAttackLockDuration = 0.35f;
	float m_basicFinalHitLockDuration = 0.7f;
	float m_basicCooldown = 0.0f;

	// Charged Attack
	float m_chargedAttackDamage = 40.0f;
	float m_chargedArcRange = 2.5f;
	float m_chargedArcAngle = 120.0f;
	float m_chargedMaxChargeTime = 2.0f;
	float m_chargedMinChargeTime = 0.5f;
	float m_chargedAttackLockDuration = 0.4f;
	float m_chargedFinalHitLockDuration = 0.8f;
	float m_chargedShotArcRange = 3.5f;
	float m_chargedShotArcAngle = 150.0f;
	float m_chargedCooldown = 0.0f;

	// Taunt
	float m_tauntDuration = 3.0f;
	float m_tauntRange = 2.5f;
	float m_tauntHalfAngleDegrees = 35.0f;
	float m_tauntLockDuration = 0.4f;
	float m_tauntCooldown = 0.0f;

	// Dash
	float m_dashDuration = 0.15f;
	float m_dashDistance = 5.0f;
	float m_dashHitWidth = 2.0f;
	float m_dashDamage = 20.0f;
	float m_dashCooldown = 0.0f;

	IMPLEMENT_DATACONTAINER_FIELDS(DeathConfig,
		FIELD_GROUP_COLLAPSE("Combo",
			SERIALIZED_FLOAT(m_comboWindow, "Combo Window R1", 0.1f, 5.0f, 0.05f),
			SERIALIZED_FLOAT(m_comboWindowR2, "Combo Window R2", 0.1f, 5.0f, 0.05f),
			SERIALIZED_FLOAT(m_comboWindowMaxCharge, "Combo Window Max Charge", 0.1f, 5.0f, 0.05f),
			SERIALIZED_FLOAT(m_comboCooldown, "Combo Cooldown", 0.0f, 5.0f, 0.1f)
		),

		FIELD_GROUP_COLLAPSE("Basic Attack",
			SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 200.0f, 1.0f),
			SERIALIZED_FLOAT(m_basicAttackRange, "Basic Attack Range", 0.5f, 10.0f, 0.1f),
			SERIALIZED_FLOAT(m_basicAttackHitAngle, "Basic Attack Hit Angle", 5.0f, 180.0f, 5.0f),
			SERIALIZED_FLOAT(m_basicAttackLockDuration, "Basic Attack Lock Duration", 0.05f, 2.0f, 0.05f),
			SERIALIZED_FLOAT(m_basicFinalHitLockDuration, "Basic Final Hit Lock Duration", 0.05f, 3.0f, 0.05f),
			SERIALIZED_FLOAT(m_basicCooldown, "Basic Cooldown", 0.0f, 10.0f, 0.01f)
		),

		FIELD_GROUP_COLLAPSE("Charged Attack",
			SERIALIZED_FLOAT(m_chargedAttackDamage, "Charged Attack Damage", 0.0f, 200.0f, 1.0f),
			SERIALIZED_FLOAT(m_chargedArcRange, "Arc Range", 0.5f, 10.0f, 0.1f),
			SERIALIZED_FLOAT(m_chargedArcAngle, "Arc Angle", 10.0f, 360.0f, 5.0f),
			SERIALIZED_FLOAT(m_chargedMaxChargeTime, "Max Charge Time", 0.5f, 5.0f, 0.1f),
			SERIALIZED_FLOAT(m_chargedMinChargeTime, "Min Charge Time", 0.0f, 3.0f, 0.05f),
			SERIALIZED_FLOAT(m_chargedAttackLockDuration, "Charged Attack Lock Duration", 0.05f, 2.0f, 0.05f),
			SERIALIZED_FLOAT(m_chargedFinalHitLockDuration, "Charged Final Hit Lock Duration", 0.05f, 3.0f, 0.05f),
			SERIALIZED_FLOAT(m_chargedShotArcRange, "Charged Arc Range", 0.5f, 10.0f, 0.1f),
			SERIALIZED_FLOAT(m_chargedShotArcAngle, "Charged Arc Angle", 10.0f, 360.0f, 5.0f),
			SERIALIZED_FLOAT(m_chargedCooldown, "Charged Cooldown", 0.0f, 10.0f, 0.01f)
		),

		FIELD_GROUP_COLLAPSE("Taunt",
			SERIALIZED_FLOAT(m_tauntDuration, "Taunt Duration", 1.0f, 10.0f, 0.05f),
			SERIALIZED_FLOAT(m_tauntRange, "Cone Range", 1.0f, 10.0f, 0.1f),
			SERIALIZED_FLOAT(m_tauntHalfAngleDegrees, "Cone Angle", 1.0f, 180.0f, 1.0f),
			SERIALIZED_FLOAT(m_tauntLockDuration, "Taunt Lock Duration", 0.05f, 2.0f, 0.05f),
			SERIALIZED_FLOAT(m_tauntCooldown, "Taunt Cooldown", 0.0f, 10.0f, 0.01f)
		),

		FIELD_GROUP_COLLAPSE("Dash",
			SERIALIZED_FLOAT(m_dashDuration, "Dash Duration", 0.0f, 1.0f, 0.01f),
			SERIALIZED_FLOAT(m_dashDistance, "Dash Distance", 0.0f, 10.0f, 0.1f),
			SERIALIZED_FLOAT(m_dashHitWidth, "Dash Hit Width", 0.1f, 5.0f, 0.05f),
			SERIALIZED_FLOAT(m_dashDamage, "Dash Damage", 0.0f, 100.0f, 1.0f),
			SERIALIZED_FLOAT(m_dashCooldown, "Dash Cooldown", 0.0f, 10.0f, 0.01f)
		)
	)
};
