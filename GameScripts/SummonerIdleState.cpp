#include "pch.h"
#include "SummonerIdleState.h"

#include "SummonerEnemyController.h"

SummonerIdleState::SummonerIdleState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SummonerIdleState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<SummonerEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_controller)
	{
		Debug::error("[SummonerIdleState] EnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SummonerIdleState] AnimationComponent not found.");
		return;
	}
	
	m_controller->clearPath();
	m_controller->resetRepathTimer();

	Debug::log("[SummonerIdleState] ENTER");
}

void SummonerIdleState::OnStateUpdate()
{
	if (!m_controller || !m_animation)
	{
		return;
	}

	if (m_controller->trySendDeathTrigger(m_animation))
	{
		return;
	}

	// can get stunned???

	m_controller->updateCurrentTarget();
	if (!m_controller->hasValidTarget())
	{
		return;
	}

	m_controller->faceCurrentTarget();

	// Summon State
	if (m_controller->isSummonReady())
	{
		AnimationAPI::sendTrigger(m_animation, "ToSummon");
		return;
	}
	
	// Teleport State
	if (m_controller->isTeleportReady())
	{
		AnimationAPI::sendTrigger(m_animation, "ToTeleport");
		return;
	}

	// Attack/Energy Ball State
	if (m_controller->isTargetInAttackRange() && m_controller->isAttackReady())
	{
		AnimationAPI::sendTrigger(m_animation, "ToEnergyBall");
		return;
	}
}

void SummonerIdleState::OnStateExit()
{
	Debug::log("[SummonerIdleState] EXIT");
}

IMPLEMENT_SCRIPT(SummonerIdleState)