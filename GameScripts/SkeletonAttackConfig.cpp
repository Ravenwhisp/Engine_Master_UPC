#include "pch.h"
#include "SkeletonAttackConfig.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(SkeletonAttackConfig, EnemyBaseAttackConfig,
	// Scimitar Strike
	FIELD_GROUP_COLLAPSE("Scimitar Strike",
		SERIALIZED_FLOAT(m_scimitarStartRange, "Scimitar Start Range", 0.0f, 20.0f, 0.1f),
		SERIALIZED_FLOAT(m_scimitarDashStopRange, "Scimitar Dash Stop Range", 0.0f, 20.0f, 0.1f),
		SERIALIZED_FLOAT(m_scimitarDashDuration, "Scimitar Dash Duration", 0.0f, 5.0f, 0.1f),
		SERIALIZED_FLOAT(m_scimitarDashSpeed, "Scimitar Dash Speed", 0.0f, 20.0f, 0.1f),
		SERIALIZED_FLOAT(m_attackClipDuration, "Attack Clip Duration", 0.0f, 5.0f, 0.05f),
		SERIALIZED_FLOAT(m_attackHitTime, "Attack Hit Time", 0.0f, 5.0f, 0.05f),
		SERIALIZED_FLOAT(m_attackAnimationSpeed, "Attack Animation Speed", 0.0f, 5.0f, 0.05f),
		SERIALIZED_FLOAT(m_stepBackDuration, "Step Back Duration", 0.0f, 5.0f, 0.1f),
		SERIALIZED_FLOAT(m_stepBackSpeed, "Step Back Speed", 0.0f, 20.0f, 0.1f),
		SERIALIZED_FLOAT(m_scimitarStunHitRange, "Scimitar Stun Hit Range", 0.0f, 10.0f, 0.1f),
		SERIALIZED_FLOAT(m_scimitarStunDuration, "Scimitar Stun Duration", 0.0f, 5.0f, 0.1f),
		SERIALIZED_FLOAT(m_scimitarHalfAngleDegrees, "Scimitar Half Angle Degrees", 0.0f, 18.0f, 1.0f)
	),
	// Guard
	FIELD_GROUP_COLLAPSE("Guard",
		SERIALIZED_FLOAT(m_guardDuration, "Guard Duration", 0.0f, 10.0f, 0.1f),
		SERIALIZED_FLOAT(m_guardCooldown, "Guard Cooldown", 0.0f, 10.0f, 0.1f),
		SERIALIZED_FLOAT(m_guardRange, "Guard Range", 0.0f, 20.0f, 0.1f),
		SERIALIZED_FLOAT(m_guardHealPerSecond, "Guard Heal Per Second", 0.0f, 100.0f, 1.0f),
		SERIALIZED_FLOAT(m_guardBlockHalfAngleDegrees, "Guard Half Angle Degrees", 0.0f, 180.0f, 1.0f)
	),
	FIELD_GROUP_COLLAPSE("Revive",
		SERIALIZED_FLOAT(m_reviveDuration, "Revive Duration", 0.0f, 20.0f, 0.01f),
		SERIALIZED_FLOAT(m_downedHP, "Downed HP", 1.0f, 150.0f, 1.0f)
	)
)

SkeletonAttackConfig::SkeletonAttackConfig(GameObject* owner)
	: EnemyBaseAttackConfig(owner)
{
}

IMPLEMENT_SCRIPT(SkeletonAttackConfig)