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

    if (!m_enemyDetectionAggro)
    {
        Debug::warn("[MeleeEnemyController] EnemyDetectionAggro not found on '%s'.", GameObjectAPI::getName(getOwner()));
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
    if (!hasValidTarget())
    {
        return false;
    }

    const PaladinAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return false;
    }

    return isCurrentTargetInRange(cfg->m_basicAttackRange);
}

bool MeleeEnemyController::playerInChargeRange() const
{
    if (!m_enemyDetectionAggro)
    {
        return false;
    }

    const PaladinAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
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
    const float chargeDistanceSquared = cfg->m_chargeRange * cfg->m_chargeRange;
    const float attackRangeSquared = cfg->m_basicAttackRange * cfg->m_basicAttackRange;

	return distanceToTargetSquared <= chargeDistanceSquared && distanceToTargetSquared > attackRangeSquared;
}

bool MeleeEnemyController::isChargeReady() const
{
	return m_chargeCooldownTimer <= 0.0f;
}

void MeleeEnemyController::consumeChargeCooldown()
{
    const PaladinAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    m_chargeCooldownTimer = cfg->m_chargeCooldown;
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

IMPLEMENT_SCRIPT_FIELDS(MeleeEnemyController,
    SERIALIZED_ASSET_REF(m_attackConfig, "Attack Config", AssetType::DATA_CONTAINER)
)
