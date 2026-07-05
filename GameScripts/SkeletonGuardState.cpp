#include "pch.h"
#include "SkeletonGuardState.h"

#include "SkeletonEnemyController.h"
#include "SkeletonAttackConfig.h"
#include "SkeletonDamageable.h"

SkeletonGuardState::SkeletonGuardState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SkeletonGuardState::OnStateEnter()
{
	m_skeletonController = GameObjectAPI::findScript<SkeletonEnemyController>(getOwner());
	m_attackConfig = GameObjectAPI::findScript<SkeletonAttackConfig>(getOwner());
	m_damageable = GameObjectAPI::findScript<SkeletonDamageable>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_skeletonController)
	{
		Debug::error("[SkeletonGuardState] SkeletonEnemyController not found.");
		return;
	}

	if (!m_attackConfig)
	{
		Debug::error("[SkeletonGuardState] SkeletonAttackConfig not found.");
		return;
	}

	if (!m_damageable)
	{
		Debug::error("[SkeletonGuardState] SkeletonDamageable not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SkeletonGuardState] AnimationComponent not found.");
		return;
	}

	m_skeletonController->clearPath();
	m_skeletonController->resetRepathTimer();
	m_skeletonController->setGuarding(true);

	m_stateTimer = 0.0f;

	Debug::log("[SkeletonGuardState] ENTER");
}

void SkeletonGuardState::OnStateUpdate()
{
	if (!m_skeletonController || !m_attackConfig || !m_damageable || !m_animation)
	{
		return;
	}

	// Go into Revive State
	if (m_skeletonController->trySendReviveTrigger(m_animation))
	{
		return;
	}

	m_skeletonController->faceCurrentTarget();

	const float dt = Time::getDeltaTime();

	m_stateTimer += dt;

	// Heal over time
	if (m_damageable->getCurrentHp() < m_damageable->getMaxHp())
	{
		const float healAmount = m_attackConfig->m_guardHealPerSecond * dt;
		m_damageable->heal(healAmount);
	}

	// End GuardState
	if (m_stateTimer >= m_attackConfig->m_guardDuration)
	{
		m_skeletonController->consumeGuardCooldown();
		AnimationAPI::sendTrigger(m_animation, "ToChase");
		return;
	}
}

void SkeletonGuardState::OnStateExit()
{
	if (m_skeletonController)
	{
		m_skeletonController->setGuarding(false);
	}

	m_stateTimer = 0.0f;

	Debug::log("[SkeletonGuardState] EXIT");
}

IMPLEMENT_SCRIPT(SkeletonGuardState)