#include "pch.h"
#include "EnemyStunnedState.h"

#include "EnemyBaseController.h"
#include "EnemySound.h"
#include "EnemyStunParticles.h"
#include "SkeletonEnemyController.h"

EnemyStunnedState::EnemyStunnedState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void EnemyStunnedState::OnStateEnter()
{
	m_controller    = GameObjectAPI::findScript<EnemyBaseController>(getOwner());
	m_skeletonController = GameObjectAPI::findScript<SkeletonEnemyController>(getOwner());
	m_animation     = AnimationAPI::getAnimationComponent(getOwner());
	m_stunParticles = GameObjectAPI::findScript<EnemyStunParticles>(getOwner());

	if (!m_controller)
	{
		Debug::error("[EnemyStunnedState] EnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[EnemyStunnedState] AnimationComponent not found.");
		return;
	}

	m_controller->clearPath();
	m_controller->resetRepathTimer();

	if (m_stunParticles) m_stunParticles->startStunParticle();
	
	m_enemySound = GameObjectAPI::findScript<EnemySound>(getOwner());
	if (m_enemySound)
	{
		m_enemySound->playStun();
	}

	Debug::log("[EnemyStunnedState] ENTER");
}

void EnemyStunnedState::OnStateUpdate()
{
	if (!m_controller || !m_animation)
	{
		return;
	}

	if (m_controller->trySendDeathTrigger(m_animation))
	{
		return;
	}

	if (m_skeletonController && m_skeletonController->trySendReviveTrigger(m_animation))
	{
		return;
	}

	if (m_stunParticles) m_stunParticles->updateStunParticle();
	m_controller->updateCurrentTarget();

	// The controller counts down the duration supplied by the attacking player.
	// Losing a target must not cancel an active stun.
	if (m_controller->isStunned())
	{
		return;
	}

	if (!m_controller->hasValidTarget())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	if (!AnimationAPI::sendTrigger(m_animation, "ToChase"))
	{
		AnimationAPI::sendTrigger(m_animation, "ToRecover");
	}
}

void EnemyStunnedState::OnStateExit()
{
	if (m_controller) m_controller->clearStun();
	if (m_stunParticles) m_stunParticles->stopStunParticle();
	Debug::log("[EnemyStunnedState] EXIT");
}

IMPLEMENT_SCRIPT(EnemyStunnedState)
