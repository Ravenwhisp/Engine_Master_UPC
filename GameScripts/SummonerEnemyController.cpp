#include "pch.h"
#include "SummonerEnemyController.h"

#include "EnemyDetectionAggro.h"
#include "SummonerAttackConfig.h"

SummonerEnemyController::SummonerEnemyController(GameObject* owner)
	: EnemyBaseController(owner)
{
}

void SummonerEnemyController::Start()
{
	m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
	if (!m_enemyDetectionAggro)
	{
		Debug::warn("[SummonerEnemyController] EnemyDetectionAggro not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	m_currentTarget = nullptr;
	m_deathTriggerSent = false;

	resetRepathTimer();
	clearPath();
}

void SummonerEnemyController::Update()
{
	const float dt = Time::getDeltaTime();

	updateCurrentTarget();

	updateTeleportCooldown(dt);
	updateSummonCooldown(dt);
	updateAttackCooldown(dt);

	updateStun(dt);
}

Transform* SummonerEnemyController::acquireCurrentTarget()
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

bool SummonerEnemyController::isTargetDowned(Transform* target) const
{
	if (!m_enemyDetectionAggro || !target)
	{
		return true;
	}

	return m_enemyDetectionAggro->isDowned(target);
}

bool SummonerEnemyController::isTargetInAttackRange() const
{
	if (!hasValidTarget() || !m_attackConfig)
	{
		return false;
	}

	return isCurrentTargetInRange(m_attackConfig.get()->m_basicAttackRange);
}

bool SummonerEnemyController::isTeleportReady() const
{
	return m_teleportCooldownTimer <= 0.0f;
}

void SummonerEnemyController::consumeTeleportCooldown()
{
	if (!m_attackConfig)
	{
		return;
	}

	m_teleportCooldownTimer = m_attackConfig.get()->m_teleportCooldown;
}

bool SummonerEnemyController::tryGetTeleportPosition(Vector3& outPosition) const
{
	constexpr int MaxTeleportAttempts = 20;

	if (!m_attackConfig || !hasValidTarget())
	{
		return false;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	Transform* targetTransform = getCurrentTarget();

	if (!ownerTransform || !targetTransform || !m_enemyDetectionAggro)
	{
		return false;
	}

	const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

	const bool targetInAttackRange =
		isCurrentTargetInRange(m_attackConfig.get()->m_basicAttackRange);

	const Vector3 searchCenter =
		targetInAttackRange ? ownerPosition : targetPosition;

	Transform* lyrielTransform = m_enemyDetectionAggro->getLyrielTransform();
	Transform* deathTransform = m_enemyDetectionAggro->getDeathTransform();

	const Vector3 searchExtents = Vector3(5.0f, 5.0f, 5.0f);
	const float attackRangeSquared =
		m_attackConfig.get()->m_basicAttackRange * m_attackConfig.get()->m_basicAttackRange;

	float bestScore = -FLT_MAX;
	bool foundPosition = false;
	Vector3 bestPosition;

	for (int i = 0; i < MaxTeleportAttempts; ++i)
	{
		Vector3 candidatePosition;

		const bool found = NavigationAPI::findRandomReachablePointAround(
			searchCenter,
			m_attackConfig.get()->m_teleportRadius,
			candidatePosition,
			searchExtents,
			1);

		if (!found)
		{
			continue;
		}

		Vector3 toTarget = candidatePosition - targetPosition;
		toTarget.y = 0.0f;

		const float targetDistance = toTarget.Length();

		if (targetDistance * targetDistance > attackRangeSquared)
		{
			continue;
		}

		float closestPlayerDistance = FLT_MAX;

		if (lyrielTransform)
		{
			Vector3 toLyriel =
				candidatePosition - TransformAPI::getGlobalPosition(lyrielTransform);

			toLyriel.y = 0.0f;

			const float distanceToLyriel = toLyriel.Length();
			if (distanceToLyriel < closestPlayerDistance)
			{
				closestPlayerDistance = distanceToLyriel;
			}
		}

		if (deathTransform)
		{
			Vector3 toDeath =
				candidatePosition - TransformAPI::getGlobalPosition(deathTransform);

			toDeath.y = 0.0f;

			const float distanceToDeath = toDeath.Length();
			if (distanceToDeath < closestPlayerDistance)
			{
				closestPlayerDistance = distanceToDeath;
			}
		}

		if (closestPlayerDistance == FLT_MAX)
		{
			continue;
		}

		if (!targetInAttackRange &&
			closestPlayerDistance < m_attackConfig.get()->m_teleportMinPlayerDistance)
		{
			continue;
		}

		float score = 0.0f;

		if (targetInAttackRange)
		{
			score = closestPlayerDistance;
		}
		else
		{
			score = -targetDistance;
		}

		if (score > bestScore)
		{
			bestScore = score;
			bestPosition = candidatePosition;
			foundPosition = true;
		}
	}

	if (foundPosition)
	{
		outPosition = bestPosition;
		return true;
	}

	return false;
}

void SummonerEnemyController::updateTeleportCooldown(float dt)
{
	if (m_teleportCooldownTimer <= 0.0f)
	{
		return;
	}
	
	m_teleportCooldownTimer -= dt;

	if (m_teleportCooldownTimer < 0.0f)
	{
		m_teleportCooldownTimer = 0.0f;
	}
}

bool SummonerEnemyController::isSummonReady() const
{
	return m_summonCooldownTimer <= 0.0f;
}

void SummonerEnemyController::consumeSummonCooldown()
{
	if (!m_attackConfig)
	{
		return;
	}

	m_summonCooldownTimer = m_attackConfig.get()->m_summonCooldown;
}

void SummonerEnemyController::summonSpidersAroundSelf()
{
	if (!m_attackConfig)
	{
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	const Vector3 ownerPosition = TransformAPI::getPosition(ownerTransform);
	const Vector3 searchExtents = Vector3(5.0f, 5.0f, 5.0f);

	for (int i = 0; i < m_attackConfig.get()->m_summonCount; ++i)
	{
		Vector3 spawnPosition;

		const bool found = NavigationAPI::findRandomReachablePointAround(
			ownerPosition,
			m_attackConfig.get()->m_summonRadius,
			spawnPosition,
			searchExtents,
			10
		);

		if (!found)
		{
			continue;
		}

		GameObjectAPI::instantiatePrefab(
			m_attackConfig.get()->m_spiderPrefab.m_id,
			spawnPosition,
			Vector3(0.0f, 0.0f, 0.0f)
		);
	}
}

void SummonerEnemyController::updateSummonCooldown(float dt)
{
	if (m_summonCooldownTimer <= 0.0f)
	{
		return;
	}

	m_summonCooldownTimer -= dt;

	if (m_summonCooldownTimer < 0.0f)
	{
		m_summonCooldownTimer = 0.0f;
	}
}

float SummonerEnemyController::getRecoveryDuration() const
{
	if (!m_attackConfig)
	{
		return 0.0f;
	}

	return m_attackConfig.get()->m_summonRecoverDuration;
}

bool SummonerEnemyController::isAttackReady() const
{
	return m_attackCooldownTimer <= 0.0f;
}

void SummonerEnemyController::consumeAttackCooldown()
{
	if (!m_attackConfig)
	{
		return;
	}

	m_attackCooldownTimer = m_attackConfig.get()->m_basicAttackCooldown;
}

void SummonerEnemyController::updateAttackCooldown(float dt)
{
	if (m_attackCooldownTimer <= 0.0f)
	{
		return;
	}

	m_attackCooldownTimer -= dt;

	if (m_attackCooldownTimer < 0.0f)
	{
		m_attackCooldownTimer = 0.0f;
	}
}

IMPLEMENT_SCRIPT_FIELDS_INHERITED(SummonerEnemyController, EnemyBaseController,
    SERIALIZED_ASSET_REF(m_attackConfig, "Attack Config", AssetType::DATA_CONTAINER)
)

IMPLEMENT_SCRIPT(SummonerEnemyController)