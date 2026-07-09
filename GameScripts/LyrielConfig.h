#pragma once

#include "ScriptAPI.h"

class LyrielConfig : public Script
{
	DECLARE_SCRIPT(LyrielConfig)

public:
	explicit LyrielConfig(GameObject* owner);

	FieldList getExposedFields() const override;

public:
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
};