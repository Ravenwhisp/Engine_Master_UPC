#pragma once

#include "DataContainerAPI.h"

class LyrielConfig : public DataContainer
{
	DECLARE_DATACONTAINER(LyrielConfig)

public:
	LyrielConfig() = default;
	explicit LyrielConfig(AssetId& id)
		: DataContainer(id)
	{
	}

	// Basic Attack
	float m_basicAttackDamage = 10.0f;
	float m_basicArrowSpeed = 18.0f;
	float m_basicAttackLockDuration = 0.2f;
	float m_basicCooldown = 0.0f;

	// Charged Attack
	float m_chargedMinDamage = 5.0f;
	float m_chargedMaxDamage = 30.0f;
	float m_chargedMaxChargeTime = 2.0f;
	float m_chargedMinAttackRange = 4.0f;
	float m_chargedMaxAttackRange = 10.0f;
	float m_chargedLineHalfWidth = 0.75f;
	float m_chargedAttackLockDuration = 0.3f;
	float m_chargedArrowSpeed = 20.0f;
	float m_chargedCooldown = 0.0f;

	// Arrow Volley
	float m_volleyDamage = 20.0f;
	float m_volleyRange = 8.0f;
	float m_volleyConeAngleDegrees = 50.0f;
	int m_volleyNumVisualArrows = 5;
	float m_volleyArrowSpeed = 18.0f;
	float m_volleyAttackLockDuration = 0.2f;
	float m_volleyCooldown = 0.0f;

	// Dash
	float m_dashDuration = 0.15f;
	float m_dashDistance = 3.0f;
	float m_dashRechargeTime = 3.0f;
	int m_dashMaxCharges = 3;
	float m_dashCooldown = 0.0f;

	// Passive / Powerup
	float m_volleyCooldownReductionPerExploit = 0.20f;

	IMPLEMENT_DATACONTAINER_FIELDS(LyrielConfig,
		FIELD_GROUP_COLLAPSE("Basic Attack",
			SERIALIZED_FLOAT(m_basicAttackDamage, "Basic Attack Damage", 0.0f, 100.0f, 0.5f),
			SERIALIZED_FLOAT(m_basicArrowSpeed, "Basic Arrow Speed", 0.0f, 100.0f, 0.5f),
			SERIALIZED_FLOAT(m_basicAttackLockDuration, "Basic Attack Lock Duration", 0.0f, 2.0f, 0.01f),
			SERIALIZED_FLOAT(m_basicCooldown, "Basic Cooldown", 0.0f, 10.0f, 0.01f)
		),

		FIELD_GROUP_COLLAPSE("Charged Attack",
			SERIALIZED_FLOAT(m_chargedMinDamage, "Charged Min Damage", 0.0f, 100.0f, 0.5f),
			SERIALIZED_FLOAT(m_chargedMaxDamage, "Charged Max Damage", 0.0f, 100.0f, 0.5f),
			SERIALIZED_FLOAT(m_chargedMaxChargeTime, "Charged Max Charge Time", 0.1f, 10.0f, 0.05f),
			SERIALIZED_FLOAT(m_chargedMinAttackRange, "Charged Min Attack Range", 0.0f, 50.0f, 0.1f),
			SERIALIZED_FLOAT(m_chargedMaxAttackRange, "Charged Max Attack Range", 0.0f, 50.0f, 0.1f),
			SERIALIZED_FLOAT(m_chargedLineHalfWidth, "Charged Line Half Width", 0.0f, 10.0f, 0.05f),
			SERIALIZED_FLOAT(m_chargedAttackLockDuration, "Charged Attack Lock Duration", 0.0f, 2.0f, 0.01f),
			SERIALIZED_FLOAT(m_chargedArrowSpeed, "Charged Arrow Speed", 0.0f, 100.0f, 0.5f),
			SERIALIZED_FLOAT(m_chargedCooldown, "Charged Cooldown", 0.0f, 10.0f, 0.01f)
		),

		FIELD_GROUP_COLLAPSE("Arrow Volley",
			SERIALIZED_FLOAT(m_volleyDamage, "Volley Damage", 0.0f, 100.0f, 0.5f),
			SERIALIZED_FLOAT(m_volleyRange, "Volley Range", 0.0f, 50.0f, 0.1f),
			SERIALIZED_FLOAT(m_volleyConeAngleDegrees, "Volley Cone Angle Degrees", 1.0f, 180.0f, 1.0f),
			SERIALIZED_INT(m_volleyNumVisualArrows, "Volley Num Visual Arrows"),
			SERIALIZED_FLOAT(m_volleyArrowSpeed, "Volley Arrow Speed", 0.0f, 100.0f, 0.5f),
			SERIALIZED_FLOAT(m_volleyAttackLockDuration, "Volley Attack Lock Duration", 0.0f, 2.0f, 0.01f),
			SERIALIZED_FLOAT(m_volleyCooldown, "Volley Cooldown", 0.0f, 10.0f, 0.01f),
			SERIALIZED_FLOAT(m_volleyCooldownReductionPerExploit, "Volley CD Reduction Per Shadow Mark Exploit", 0.0f, 1.0f, 0.05f)
		),

		FIELD_GROUP_COLLAPSE("Dash",
			SERIALIZED_FLOAT(m_dashDuration, "Dash Duration", 0.0f, 1.0f, 0.01f),
			SERIALIZED_FLOAT(m_dashDistance, "Dash Distance", 0.0f, 10.0f, 0.1f),
			SERIALIZED_FLOAT(m_dashRechargeTime, "Dash Recharge Time", 0.1f, 10.0f, 0.1f),
			SERIALIZED_INT(m_dashMaxCharges, "Dash Max Charges"),
			SERIALIZED_FLOAT(m_dashCooldown, "Dash Cooldown", 0.0f, 10.0f, 0.01f)
		)
	)
};
