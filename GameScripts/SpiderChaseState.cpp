#include "pch.h"
#include "SpiderChaseState.h"

#include "SpiderEnemyController.h"

SpiderChaseState::SpiderChaseState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SpiderChaseState::OnStateEnter()
{
	m_spiderController = GameObjectAPI::findScript<SpiderEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_spiderController)
	{
		Debug::error("[SpiderChaseState] SpiderEnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SpiderChaseState] AnimationComponent not found.");
		return;
	}

	m_spiderController->clearPath();
	m_spiderController->resetRepathTimer();

	Debug::log("[SpiderChaseState] ENTER");
}

void SpiderChaseState::OnStateUpdate()
{
	if (!m_spiderController || !m_animation)
	{
		return;
	}

	if (m_spiderController->trySendDeathTrigger(m_animation))
	{
		return;
	}

	if (!m_spiderController->hasValidTarget())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		Debug::log("[SpiderChaseState] Idle trigger sent");
		return;
	}

	if (m_spiderController->isTargetInAttackRange())
	{
		AnimationAPI::sendTrigger(m_animation, "ToAttack");
		Debug::log("[SpiderChaseState] Attack trigger sent");
		return;
	}

	m_spiderController->moveTowardsTarget();
}

void SpiderChaseState::OnStateExit()
{
	Debug::log("[SpiderChaseState] EXIT");

	if (!m_spiderController)
	{
		return;
	}

	m_spiderController->clearPath();
	m_spiderController->resetRepathTimer();
}

IMPLEMENT_SCRIPT(SpiderChaseState)