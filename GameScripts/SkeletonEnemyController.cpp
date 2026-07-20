#include "pch.h"
#include "SkeletonEnemyController.h"

#include "EnemyDetectionAggro.h"
#include "SkeletonAttackConfig.h"
#include "SkeletonDamageable.h"

#include <cmath>

SkeletonEnemyController::SkeletonEnemyController(GameObject* owner)
	: EnemyBaseController(owner)
{
}

void SkeletonEnemyController::Start()
{
	m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
	m_damageable = GameObjectAPI::findScript<SkeletonDamageable>(getOwner());

	if (!m_enemyDetectionAggro)
	{
		Debug::warn("[SkeletonEnemyController] EnemyDetectionAggro not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	if (!m_damageable)
	{
		Debug::warn("[SkeletonEnemyController] SkeletonDamageable not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	m_currentTarget = nullptr;
	m_deathTriggerSent = false;

	resetRepathTimer();
	clearPath();
}

void SkeletonEnemyController::Update()
{
	const float dt = Time::getDeltaTime();

	updateCurrentTarget();
	updateGuardCooldown(dt);
	//updateStun(dt);
}

Transform* SkeletonEnemyController::acquireCurrentTarget()
{
	if (!m_enemyDetectionAggro)
	{
		m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
	}

	if (!m_enemyDetectionAggro)
	{
		return nullptr;
	}

	return m_enemyDetectionAggro->getCurrentTarget();
}

bool SkeletonEnemyController::isTargetDowned(Transform* target) const
{
	if (!m_enemyDetectionAggro || !target)
	{
		return true;
	}

	return m_enemyDetectionAggro->isDowned(target);
}

bool SkeletonEnemyController::isTargetInScimitarRange() const
{
	if (!hasValidTarget() || !m_attackConfig.get())
	{
		return false;
	}

	return isCurrentTargetInRange(m_attackConfig.get()->m_scimitarStartRange);
}

bool SkeletonEnemyController::isGuardReady() const
{
	return m_guardCooldownTimer <= 0.0f;
}

void SkeletonEnemyController::consumeGuardCooldown()
{
	if (!m_attackConfig.get())
	{
		return;
	}

	m_guardCooldownTimer = m_attackConfig.get()->m_guardCooldown;
}

void SkeletonEnemyController::updateGuardCooldown(float dt)
{
	if (m_guardCooldownTimer <= 0.0f)
	{
		return;
	}

	m_guardCooldownTimer -= dt;

	if (m_guardCooldownTimer < 0.0f)
	{
		m_guardCooldownTimer = 0.0f;
	}
}

bool SkeletonEnemyController::shouldUseGuard() const
{
	if (!hasValidTarget() || !m_attackConfig.get())
	{
		return false;
	}

	if (!isGuardReady())
	{
		return false;
	}

	if (!isCurrentTargetInRange(m_attackConfig.get()->m_guardRange))
	{
		return false;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform || !m_currentTarget)
	{
		return false;
	}

	Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	Vector3 targetPosition = TransformAPI::getGlobalPosition(m_currentTarget);

	Vector3 toTarget = targetPosition - ownerPosition;
	toTarget.y = 0.0f;

	if (toTarget.LengthSquared() <= 0.0001f)
	{
		return true;
	}

	Vector3 forward = TransformAPI::getForward(ownerTransform);
	forward.y = 0.0f;

	if (forward.LengthSquared() <= 0.0001f)
	{
		return false;
	}

	toTarget.Normalize();
	forward.Normalize();

	float dot = forward.Dot(toTarget);

	constexpr float degreesToRadians = 3.14159265f / 180.0f;
	const float minDot = std::cos(m_attackConfig.get()->m_guardBlockHalfAngleDegrees * degreesToRadians);

	return dot >= minDot;
}

bool SkeletonEnemyController::isGuarding() const
{
	return m_isGuarding;
}

void SkeletonEnemyController::setGuarding(bool guarding)
{
	m_isGuarding = guarding;
}

bool SkeletonEnemyController::isDowned() const
{
	return m_damageable && m_damageable->isDowned();
}

bool SkeletonEnemyController::isPermanentlyDead() const
{
	return m_damageable && m_damageable->isPermanentlyDead();
}

bool SkeletonEnemyController::trySendReviveTrigger(AnimationComponent* animation)
{
	if (!animation)
	{
		return false;
	}

	if (!isDowned())
	{
		return false;
	}

	if (!AnimationAPI::sendTrigger(animation, "ToRevive"))
	{
		return false;
	}

	clearPath();
	resetRepathTimer();

	Debug::log("[SkeletonEnemyController] ToRevive trigger sent.");

	return true;
}

IMPLEMENT_SCRIPT_FIELDS_INHERITED(SkeletonEnemyController, EnemyBaseController,
    SERIALIZED_ASSET_REF(m_attackConfig, "Attack Config", AssetType::DATA_CONTAINER)
)

IMPLEMENT_SCRIPT(SkeletonEnemyController)