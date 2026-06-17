#pragma once

#include "ScriptAPI.h"

class DeathConfig : public Script
{
	DECLARE_SCRIPT(DeathConfig)

public:
	explicit DeathConfig(GameObject* owner);

	ScriptFieldList getExposedFields() const override;

public:
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
	float m_tauntCooldown = 0.0f;

	// Dash
	float m_dashDuration = 0.15f;
	float m_dashDistance = 5.0f;
	float m_dashHitWidth = 2.0f;
	float m_dashDamage = 20.0f;
	float m_dashCooldown = 0.0f;
};