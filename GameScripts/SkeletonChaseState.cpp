#include "pch.h"
#include "SkeletonChaseState.h"

#include "SkeletonEnemyController.h"

SkeletonChaseState::SkeletonChaseState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SkeletonChaseState::OnStateEnter()
{
	m_skeletonController = GameObjectAPI::findScript<SkeletonEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_skeletonController)
	{
		Debug::error("[SkeletonChaseState] SkeletonEnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SkeletonChaseState] AnimationComponent not found.");
		return;
	}

	m_skeletonController->clearPath();
	m_skeletonController->resetRepathTimer();

	Debug::log("[SkeletonChaseState] ENTER");
}

void SkeletonChaseState::OnStateUpdate()
{
	if (!m_skeletonController || !m_animation)
	{
		return;
	}

	if (m_skeletonController->trySendReviveTrigger(m_animation))
	{
		return;
	}

	// stunned? //

	if (!m_skeletonController->hasValidTarget())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	// Guard
	if (m_skeletonController->shouldUseGuard())
	{
		AnimationAPI::sendTrigger(m_animation, "ToGuard");
		return;
	}

	// Scimitar Strike
	if (m_skeletonController->isTargetInScimitarRange())
	{
		AnimationAPI::sendTrigger(m_animation, "ToScimitar");
		return;
	}

	m_skeletonController->moveTowardsTarget();
}

void SkeletonChaseState::OnStateExit()
{
	Debug::log("[SkeletonChaseState] EXIT");

	if (!m_skeletonController)
	{
		return;
	}

	m_skeletonController->clearPath();
	m_skeletonController->resetRepathTimer();
}

IMPLEMENT_SCRIPT(SkeletonChaseState)