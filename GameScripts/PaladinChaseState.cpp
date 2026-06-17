#include "pch.h"
#include "PaladinChaseState.h"

#include "MeleeEnemyController.h"

PaladinChaseState::PaladinChaseState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void PaladinChaseState::OnStateEnter()
{
	m_paladinController = GameObjectAPI::findScript<MeleeEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_paladinController)
	{
		Debug::error("[PaladinChaseState] MeleeEnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[PaladinChaseState] AnimationComponent not found.");
		return;
	}

	m_paladinController->clearPath();
	m_paladinController->resetRepathTimer();

	Debug::log("[PaladinChaseState] ENTER");
}

void PaladinChaseState::OnStateUpdate()
{
	if (!m_paladinController || !m_animation)
	{
		return;
	}

	if (m_paladinController->trySendDeathTrigger(m_animation))
	{
		return;
	}

	if (m_paladinController->trySendStunTrigger(m_animation))
	{
		return;
	}

	if (!m_paladinController->hasValidTarget())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		Debug::log("[PaladinChaseState] Idle trigger sent");
		return;
	}

	if (m_paladinController->playerInChargeRange() && m_paladinController->isChargeReady())
	{
		AnimationAPI::sendTrigger(m_animation, "ToCharge");
		Debug::log("[PaladinChaseState] Charge trigger sent");
		return;
	}

	if (m_paladinController->isTargetInAttackRange())
	{
		AnimationAPI::sendTrigger(m_animation, "ToAttack");
		Debug::log("[PaladinChaseState] Attack trigger sent");
		return;
	}

	m_paladinController->moveTowardsTarget();
}

void PaladinChaseState::OnStateExit()
{
	Debug::log("[PaladinChaseState] EXIT");

	if (!m_paladinController)
	{
		return;
	}

	m_paladinController->clearPath();
	m_paladinController->resetRepathTimer();
}

IMPLEMENT_SCRIPT(PaladinChaseState)