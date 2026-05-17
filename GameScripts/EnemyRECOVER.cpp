#include "pch.h"
#include "EnemyRECOVER.h"
#include "EnemyController.h"

IMPLEMENT_SCRIPT_FIELDS(EnemyRECOVER,
	SERIALIZED_FLOAT(m_recoverDuration, "Recover Duration", 0.0f, 10.0f, 0.1f),
	SERIALIZED_BOOL(m_goToStunAfterRecover, "Go To Stun After Recover"),
	SERIALIZED_BOOL(m_debugEnabled, "Debug Enabled")
)

EnemyRECOVER::EnemyRECOVER(GameObject* owner) : StateMachineScript(owner)
{
}

void EnemyRECOVER::OnStateEnter()
{
	m_enemyController = GameObjectAPI::findScript<EnemyController>(getOwner());

	m_recoverTimer = 0.0f;

	if (m_debugEnabled)
	{
		Debug::log("[EnemyRECOVER] ENTER");
	}
}

void EnemyRECOVER::OnStateUpdate()
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

	m_enemyController->updateCurrentTarget();

	if (!m_enemyController->hasValidTarget())
	{
		AnimationAPI::playState(animation, "Idle");
		return;
	}

	if (!m_enemyController->isTargetInCombatRange())
	{
		AnimationAPI::playState(animation, "Chase");
		return;
	}

	m_enemyController->faceCurrentTarget();

	m_recoverTimer += Time::getDeltaTime();

	if (m_recoverTimer >= m_recoverDuration)
	{
		if (m_goToStunAfterRecover)
		{
			AnimationAPI::playState(animation, "Stun");
		}
		else
		{
			AnimationAPI::playState(animation, "Attack");
		}

		return;
	}
}

void EnemyRECOVER::OnStateExit()
{
	if (m_debugEnabled)
	{
		Debug::log("[EnemyRECOVER] EXIT");
	}
}

IMPLEMENT_SCRIPT(EnemyRECOVER)