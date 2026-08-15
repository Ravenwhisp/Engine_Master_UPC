#include "pch.h"
#include "PaladinChaseState.h"

#include "MeleeEnemyController.h"
#include "PaladinVFX.h"

PaladinChaseState::PaladinChaseState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void PaladinChaseState::OnStateEnter()
{
	m_paladinController = GameObjectAPI::findScript<MeleeEnemyController>(getOwner());
	m_paladinVFX = GameObjectAPI::findScript<PaladinVFX>(getOwner());
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

	if (!m_paladinVFX)
	{
		Debug::warn("[PaladinChaseState] PaladinVFX not found.");
	}

	stopWalkingDust();

	m_paladinController->clearPath();
	m_paladinController->resetRepathTimer();

	Debug::log("[PaladinChaseState] ENTER");
}

void PaladinChaseState::OnStateUpdate()
{
	if (!m_paladinController || !m_animation)
	{
		stopWalkingDust();
		return;
	}

	if (m_paladinController->trySendDeathTrigger(m_animation))
	{
		stopWalkingDust();
		return;
	}

	if (m_paladinController->trySendStunTrigger(m_animation))
	{
		stopWalkingDust();
		return;
	}

	if (!m_paladinController->hasValidTarget())
	{
		stopWalkingDust();

		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		Debug::log("[PaladinChaseState] Idle trigger sent");
		return;
	}

	if (m_paladinController->playerInChargeRange() && m_paladinController->isChargeReady())
	{
		stopWalkingDust();

		AnimationAPI::sendTrigger(m_animation, "ToCharge");
		Debug::log("[PaladinChaseState] Charge trigger sent");
		return;
	}

	if (m_paladinController->isTargetInAttackRange())
	{
		stopWalkingDust();

		AnimationAPI::sendTrigger(m_animation, "ToAttack");
		Debug::log("[PaladinChaseState] Attack trigger sent");
		return;
	}

	const bool moved = m_paladinController->moveTowardsTarget();

	if (m_paladinVFX)
	{
		m_paladinVFX->setWalkingDustActive(moved);
	}
}

void PaladinChaseState::OnStateExit()
{
	Debug::log("[PaladinChaseState] EXIT");

	stopWalkingDust();

	if (!m_paladinController)
	{
		return;
	}

	m_paladinController->clearPath();
	m_paladinController->resetRepathTimer();
}

void PaladinChaseState::stopWalkingDust()
{
	if (m_paladinVFX)
	{
		m_paladinVFX->setWalkingDustActive(false);
	}
}

IMPLEMENT_SCRIPT(PaladinChaseState)