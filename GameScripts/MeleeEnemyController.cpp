#include "pch.h"
#include "MeleeEnemyController.h"

#include "EnemyDetectionAggro.h"
#include "PaladinAttackConfig.h"

#include "Damageable.h"

MeleeEnemyController::MeleeEnemyController(GameObject* owner)
	: EnemyBaseController(owner)
{
}

void MeleeEnemyController::Start()
{
	m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
	m_attackConfig = GameObjectAPI::findScript<PaladinAttackConfig>(getOwner());

	if (!m_enemyDetectionAggro)
	{
		Debug::warn("[MeleeEnemyController] EnemyDetectionAggro not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	if (!m_attackConfig)
	{
		Debug::warn("[MeleeEnemyController] PaladinAttackConfig not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	m_currentTarget = nullptr;
	m_deathTriggerSent = false;

	resetRepathTimer();
	clearPath();
}

void MeleeEnemyController::Update()
{
	const float dt = Time::getDeltaTime();

	updateCurrentTarget();
	updateChargeCooldown(dt);
	updateStun(dt);
}

Transform* MeleeEnemyController::acquireCurrentTarget()
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

bool MeleeEnemyController::isTargetDowned(Transform* target) const
{
	if (!m_enemyDetectionAggro || !target)
	{
		return true;
	}

	return m_enemyDetectionAggro->isDowned(target);
}

bool MeleeEnemyController::isTargetInAttackRange() const
{
	if (!hasValidTarget() || !m_attackConfig)
	{
		return false;
	}

	return isCurrentTargetInRange(m_attackConfig->m_basicAttackRange);
}

bool MeleeEnemyController::playerInChargeRange() const
{
	if (!m_enemyDetectionAggro || !m_attackConfig)
	{
		return false;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return false;
	}

	if (!m_currentTarget)
	{
		return false;
	}

	Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	Vector3 targetPosition = TransformAPI::getGlobalPosition(m_currentTarget);

	Vector3 difference = targetPosition - ownerPosition;
	difference.y = 0.0f;

	const float distanceToTargetSquared = difference.LengthSquared();
	const float chargeDistanceSquared = m_attackConfig->m_chargeRange * m_attackConfig->m_chargeRange;
	const float attackRangeSquared = m_attackConfig->m_basicAttackRange * m_attackConfig->m_basicAttackRange;

	return distanceToTargetSquared <= chargeDistanceSquared && distanceToTargetSquared > attackRangeSquared;
}

bool MeleeEnemyController::isChargeReady() const
{
	return m_chargeCooldownTimer <= 0.0f;
}

void MeleeEnemyController::consumeChargeCooldown()
{
	if (!m_attackConfig)
	{
		return;
	}

	m_chargeCooldownTimer = m_attackConfig->m_chargeCooldown;
}

void MeleeEnemyController::updateChargeCooldown(float dt)
{
	if (m_chargeCooldownTimer <= 0.0f)
	{
		return;
	}

	m_chargeCooldownTimer -= dt;

	if (m_chargeCooldownTimer < 0.0f)
	{
		m_chargeCooldownTimer = 0.0f;
	}
}

Vector3 MeleeEnemyController::getChargeDirection() const
{
	if (!m_enemyDetectionAggro || !m_currentTarget)
	{
		return Vector3::Zero;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	Vector3 direction = TransformAPI::getForward(ownerTransform);
	Vector3 targetPosition = TransformAPI::getGlobalPosition(m_currentTarget);

	direction = targetPosition - ownerPosition;
	direction.y = 0.0f;

	if (direction.LengthSquared() > 0.0001f)
	{
		direction.Normalize();
	}

	return direction;
}

IMPLEMENT_SCRIPT(MeleeEnemyController)
