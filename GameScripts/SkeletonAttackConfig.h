#pragma once

#include "EnemyBaseAttackConfig.h"

class SkeletonAttackConfig : public EnemyBaseAttackConfig
{
	DECLARE_SCRIPT(SkeletonAttackConfig)

public:
	explicit SkeletonAttackConfig(GameObject* owner);

	ScriptFieldList getExposedFields() const override;

public:
	// --- Scimitar Strike --- //
	// Dash
	float m_scimitarStartRange = 4.5f;
	float m_scimitarDashStopRange = 1.5f;
	float m_scimitarDashDuration = 2.0f;
	float m_scimitarDashSpeed = 10.0f;
	float m_scimitarHalfAngleDegrees = 55.0f;

	// Attack
	float m_attackClipDuration = 1.33f;
	float m_attackHitTime = 0.45f;
	float m_attackAnimationSpeed = 1.3f;

	// Backstep
	float m_stepBackDuration = 0.15f;
	float m_stepBackSpeed = 4.0f;

	// Final Strike
	float m_scimitarStunHitRange = 1.5f;
	float m_scimitarStunDuration = 3.0f;

	// --- Guard --- //
	float m_guardDuration = 5.0f;
	float m_guardCooldown = 10.0f;
	float m_guardRange = 3.5f;
	float m_guardHealPerSecond = 2.5f;
	float m_guardBlockHalfAngleDegrees = 60.0f;

	// Revive
	float m_reviveDuration = 4.0f;
	float m_downedHP = 40.0f;
};