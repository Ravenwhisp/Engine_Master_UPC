#include "pch.h"
#include "EnemyCHASE.h"
#include "EnemyController.h"

IMPLEMENT_SCRIPT_FIELDS(EnemyCHASE,
	SERIALIZED_BOOL(m_useCharge, "Use Charge"),
	SERIALIZED_FLOAT(m_chargeTriggerRange, "Charge Trigger Range", 0.0f, 50.0f, 0.1f),
	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled")
)

EnemyCHASE::EnemyCHASE(GameObject* owner) : StateMachineScript(owner)
{
}

void EnemyCHASE::OnStateEnter()
{
	m_enemyController = GameObjectAPI::findScript<EnemyController>(getOwner());

	if (!m_enemyController)
	{
		return;
	}

	m_enemyController->clearPath();
	m_enemyController->resetRepathTimer();

	if (m_debugEnabled)
	{
		Debug::log("[EnemyCHASE] ENTER");
	}

	m_enemyController->updateCurrentTarget();

	if (m_enemyController->hasValidTarget())
	{
		m_enemyController->buildPathToTarget();
	}
}

void EnemyCHASE::OnStateUpdate()
{
	if (!m_enemyController)
	{
		return;
	}

	AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
	if (!animation)
	{
		return;
	}

	m_enemyController->tickChargeCooldown(Time::getDeltaTime());
	m_enemyController->updateCurrentTarget();

	if (!m_enemyController->hasValidTarget())
	{
		AnimationAPI::playState(animation, "Idle"); // sendTrigger
		return;
	}

	Transform* currentTarget = m_enemyController->getCurrentTarget();

	if (m_useCharge && currentTarget && m_enemyController->isChargeReady())
	{
		Vector3 ownerPosition = getOwner()->GetTransform()->getPosition();
		Vector3 targetPosition = currentTarget->getPosition();

		Vector3 difference = targetPosition - ownerPosition;
		difference.y = 0.0f;

		float distanceToTarget = difference.Length();

		if (distanceToTarget <= m_chargeTriggerRange && distanceToTarget > m_enemyController->m_combatRange)
		{
			m_enemyController->faceCurrentTarget();
			AnimationAPI::playState(animation, "Charge");
			return;
		}
	}

	if (m_enemyController->isTargetInAttackEnterRange())
	{
		m_enemyController->faceCurrentTarget();
		AnimationAPI::playState(animation, "Attack");
		return;
	}

	m_enemyController->addToRepathTimer(Time::getDeltaTime());

	if (m_enemyController->shouldRepath())
	{
		m_enemyController->buildPathToTarget();
		m_enemyController->resetRepathTimer();
	}

	m_enemyController->followPath();
}

void EnemyCHASE::OnStateExit()
{
	if (!m_debugEnabled)
	{
		return;
	}

	Debug::log("[EnemyCHASE] EXIT");

	if (!m_enemyController)
	{
		return;
	}

	m_enemyController->clearPath();
	m_enemyController->resetRepathTimer();
}

IMPLEMENT_SCRIPT(EnemyCHASE)