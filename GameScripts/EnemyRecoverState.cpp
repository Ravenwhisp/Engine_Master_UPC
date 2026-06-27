#include "pch.h"
#include "EnemyRecoverState.h"

#include "EnemyBaseController.h"

EnemyRecoverState::EnemyRecoverState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void EnemyRecoverState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<EnemyBaseController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_controller)
	{
		Debug::error("[EnemyRecoverState] EnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[EnemyRecoverState] AnimationComponent not found.");
		return;
	}

	m_controller->clearPath();
	m_controller->resetRepathTimer();
	m_stateTimer = 0.0f;

	Debug::log("[EnemyRecoverState] ENTER");
}

void EnemyRecoverState::OnStateUpdate()
{
	if (!m_controller || !m_animation)
	{
		return;
	}

	if (m_controller->trySendDeathTrigger(m_animation))
	{
		return;
	}

	if (m_controller->trySendStunTrigger(m_animation))
	{
		return;
	}

	m_stateTimer += Time::getDeltaTime();

	m_controller->updateCurrentTarget();
	if (!m_controller->hasValidTarget())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	if (m_stateTimer >= m_controller->getRecoveryDuration())
	{
		AnimationAPI::sendTrigger(m_animation, "ToChase");
		return;
	}
}

void EnemyRecoverState::OnStateExit()
{
	m_stateTimer = 0.0f;
	Debug::log("[EnemyRecoverState] EXIT");
}

IMPLEMENT_SCRIPT(EnemyRecoverState)