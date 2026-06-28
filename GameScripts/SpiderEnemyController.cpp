#include "pch.h"
#include "SpiderEnemyController.h"

#include "EnemyDetectionAggro.h"
#include "EnemyBaseAttackConfig.h"

#include "Damageable.h"

SpiderEnemyController::SpiderEnemyController(GameObject* owner)
	: EnemyBaseController(owner)
{
}

void SpiderEnemyController::Start()
{
	m_enemyDetectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
	m_attackConfig = GameObjectAPI::findScript<EnemyBaseAttackConfig>(getOwner());

	if (!m_enemyDetectionAggro)
	{
		Debug::warn("[SpiderEnemyController] EnemyDetectionAggro not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	if (!m_attackConfig)
	{
		Debug::warn("[SpiderEnemyController] EnemyBaseAttackConfig not found on '%s'.", GameObjectAPI::getName(getOwner()));
	}

	m_currentTarget = nullptr;
	m_deathTriggerSent = false;

	resetRepathTimer();
	clearPath();
}

void SpiderEnemyController::Update()
{
	updateCurrentTarget();
}

Transform* SpiderEnemyController::acquireCurrentTarget()
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

bool SpiderEnemyController::isTargetDowned(Transform* target) const
{
	if (!m_enemyDetectionAggro || !target)
	{
		return true;
	}

	return m_enemyDetectionAggro->isDowned(target);
}

bool SpiderEnemyController::isTargetInAttackRange() const
{
	if (!hasValidTarget() || !m_attackConfig)
	{
		return false;
	}
	
	return isCurrentTargetInRange(m_attackConfig->m_basicAttackRange);
}

IMPLEMENT_SCRIPT(SpiderEnemyController)