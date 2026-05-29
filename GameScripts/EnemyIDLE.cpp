#include "pch.h"
#include "EnemyIDLE.h"
#include "EnemyController.h"

IMPLEMENT_SCRIPT_FIELDS(EnemyIDLE,
	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled")
)

EnemyIDLE::EnemyIDLE(GameObject* owner) : StateMachineScript(owner)
{
}

void EnemyIDLE::OnStateEnter()
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
		Debug::log("[EnemyIDLE] ENTER");
	}
}

void EnemyIDLE::OnStateUpdate()
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

	if (m_enemyController->isDead())
	{
		m_enemyController->clearPath();
		AnimationAPI::sendTrigger(animation, "ToDeath");
		return;
	}

	m_enemyController->updateCurrentTarget();

	if (!m_enemyController->hasValidTarget())
	{
		return;
	}

	if (m_enemyController->isTargetInCombatRange())
	{
		m_enemyController->faceCurrentTarget();
		AnimationAPI::playState(animation, "Attack");
		return;
	}

	AnimationAPI::playState(animation, "Chase");
}

void EnemyIDLE::OnStateExit()
{
	if (!m_debugEnabled)
	{
		return;
	}

	Debug::log("[EnemyIDLE] EXIT");
}

IMPLEMENT_SCRIPT(EnemyIDLE)