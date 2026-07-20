#include "pch.h"
#include "SkeletonReviveState.h"

#include "SkeletonEnemyController.h"
#include "SkeletonDamageable.h"
#include "SkeletonAttackConfig.h"

SkeletonReviveState::SkeletonReviveState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SkeletonReviveState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<SkeletonEnemyController>(getOwner());
	m_damageable = GameObjectAPI::findScript<SkeletonDamageable>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_controller)
	{
		Debug::error("[SkeletonReviveState] SkeletonEnemyController not found.");
		return;
	}

	if (!m_damageable)
	{
		Debug::error("[SkeletonReviveState] SkeletonDamageable not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SkeletonReviveState] AnimationComponent not found.");
		return;
	}

	m_controller->clearPath();
	m_controller->resetRepathTimer();

	m_phaseTimer = 0.0f;
	m_reviveTimer = 0.0f;

	changePhase(Phase::ReviveStart);

	Debug::log("[SkeletonReviveState] ENTER");
}

void SkeletonReviveState::OnStateUpdate()
{
	if (!m_controller || !m_damageable || !m_attackConfig.get() || !m_animation)
	{
		return;
	}

	if (m_damageable->isPermanentlyDead())
	{
		goToDeath();
		return;
	}

	m_phaseTimer += Time::getDeltaTime();

	if (m_phase == Phase::ReviveStart)
	{
		updateReviveStart();
		return;
	}

	if (m_phase == Phase::ReviveIdle)
	{
		updateReviveIdle();
		return;
	}

	if (m_phase == Phase::ReviveEnd)
	{
		updateReviveEnd();
		return;
	}
}

void SkeletonReviveState::OnStateExit()
{
	if (m_animation)
	{
		AnimationAPI::clearOverrideClip(m_animation, 0.0f);
	}

	m_phaseTimer = 0.0f;
	m_reviveTimer = 0.0f;

	Debug::log("[SkeletonReviveState] EXIT");
}

void SkeletonReviveState::changePhase(Phase phase)
{
	m_phase = phase;
	m_phaseTimer = 0.0f;

	if (!m_animation)
	{
		return;
	}

	if (phase == Phase::ReviveStart)
	{
		AnimationAPI::playOverrideClip(m_animation, "Skeleton_Revive_Start", 0.0f, false);
		return;
	}

	if (phase == Phase::ReviveIdle)
	{
		AnimationAPI::playOverrideClip(m_animation, "Skeleton_Revive_Idle", 0.0f, false);
		return;
	}

	if (phase == Phase::ReviveEnd)
	{
		AnimationAPI::playOverrideClip(m_animation, "Skeleton_Revive_End", 0.0f, false);
		return;
	}
}

void SkeletonReviveState::updateReviveStart()
{
	if (m_phaseTimer >= AnimationAPI::getPlaybackDuration(m_animation))
	{
		changePhase(Phase::ReviveIdle);
		return;
	}
}

void SkeletonReviveState::updateReviveIdle()
{
	if (m_phaseTimer >= m_attackConfig.get()->m_reviveDuration)
	{
		changePhase(Phase::ReviveEnd);
		return;
	}
}

void SkeletonReviveState::updateReviveEnd()
{
	if (m_phaseTimer >= AnimationAPI::getPlaybackDuration(m_animation))
	{
		m_damageable->completeRevive();
		goToChase();
		return;
	}
}

void SkeletonReviveState::goToDeath()
{
	if (!m_animation)
	{
		return;
	}

	AnimationAPI::clearOverrideClip(m_animation, 0.0f);
	AnimationAPI::sendTrigger(m_animation, "ToDeath");
}

void SkeletonReviveState::goToChase()
{
	if (!m_animation)
	{
		return;
	}

	AnimationAPI::clearOverrideClip(m_animation, 0.0f);
	AnimationAPI::sendTrigger(m_animation, "ToChase");
}

IMPLEMENT_SCRIPT(SkeletonReviveState)