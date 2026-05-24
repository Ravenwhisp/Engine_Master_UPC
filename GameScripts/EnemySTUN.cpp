#include "pch.h"
#include "EnemySTUN.h"
#include "EnemyController.h"

IMPLEMENT_SCRIPT_FIELDS(EnemySTUN,
	SERIALIZED_FLOAT(m_stunDuration, "Stun Duration", 0.0f, 10.0f, 0.1f),
	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled")
)

EnemySTUN::EnemySTUN(GameObject* owner)
	: StateMachineScript(owner)
{
}

void EnemySTUN::OnStateEnter()
{
	m_elapsedTime = 0.0f;

	m_enemyController = GameObjectAPI::findScript<EnemyController>(getOwner());

	if (m_enemyController)
	{
		m_enemyController->clearPath();
		m_enemyController->resetRepathTimer();
	}

	if (m_debugEnabled)
	{
		Debug::log("[EnemySTUN] ENTER");
	}
}

void EnemySTUN::OnStateUpdate()
{
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

	m_elapsedTime += Time::getDeltaTime();

	if (m_elapsedTime < m_stunDuration)
	{
		return;
	}

	if (!m_enemyController)
	{
		AnimationAPI::playState(animation, "Idle");
		return;
	}

	m_enemyController->updateCurrentTarget();

	if (!m_enemyController->hasValidTarget())
	{
		AnimationAPI::playState(animation, "Idle");
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

void EnemySTUN::OnStateExit()
{
	if (m_debugEnabled)
	{
		Debug::log("[EnemySTUN] EXIT");
	}
}

IMPLEMENT_SCRIPT(EnemySTUN)