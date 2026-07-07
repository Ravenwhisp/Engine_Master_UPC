#include "pch.h"
#include "EnemyStunnedState.h"

#include "EnemyBaseController.h"
#include "EnemySound.h"
#include "EnemyStunParticles.h"

EnemyStunnedState::EnemyStunnedState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void EnemyStunnedState::OnStateEnter()
{
	m_controller    = GameObjectAPI::findScript<EnemyBaseController>(getOwner());
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
	m_stateTimer = 0.0f;

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

	m_stateTimer += Time::getDeltaTime();
	if (m_stunParticles) m_stunParticles->updateStunParticle();
	m_controller->updateCurrentTarget();

	if (!m_controller->hasValidTarget())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	if (m_stateTimer >= m_controller->getStunnedDuration())
	{
		AnimationAPI::sendTrigger(m_animation, "ToChase");
		return;
	}
}

void EnemyStunnedState::OnStateExit()
{
	m_stateTimer = 0.0f;
	if (m_stunParticles) m_stunParticles->stopStunParticle();
	Debug::log("[EnemyStunnedState] EXIT");
}

IMPLEMENT_SCRIPT(EnemyStunnedState)