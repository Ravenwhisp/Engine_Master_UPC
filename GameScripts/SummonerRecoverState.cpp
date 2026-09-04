#include "pch.h"
#include "SummonerRecoverState.h"

#include "SummonerEnemyController.h"

SummonerRecoverState::SummonerRecoverState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SummonerRecoverState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<SummonerEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_controller)
	{
		Debug::error("[SummonerRecoverState] EnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SummonerRecoverState] AnimationComponent not found.");
		return;
	}

	m_controller->clearPath();
	m_controller->resetRepathTimer();
	m_stateTimer = 0.0f;

	Debug::log("[SummonerRecoverState] ENTER");
}

void SummonerRecoverState::OnStateUpdate()
{
	if (!m_controller || !m_animation)
	{
		return;
	}

	if (m_controller->trySendDeathTrigger(m_animation))
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
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}
}

void SummonerRecoverState::OnStateExit()
{
	m_stateTimer = 0.0f;
	Debug::log("[SummonerRecoverState] EXIT");
}

IMPLEMENT_SCRIPT(SummonerRecoverState)