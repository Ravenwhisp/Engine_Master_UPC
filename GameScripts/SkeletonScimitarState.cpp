#include "pch.h"
#include "SkeletonScimitarState.h"

#include "SkeletonEnemyController.h"
#include "SkeletonAttackConfig.h"
#include "EnemyAttackExecutor.h"

SkeletonScimitarState::SkeletonScimitarState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SkeletonScimitarState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<SkeletonEnemyController>(getOwner());
	m_attackConfig = GameObjectAPI::findScript<SkeletonAttackConfig>(getOwner());
	m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_controller)
	{
		Debug::error("[SkeletonScimitarState] SkeletonEnemyController not found.");
		return;
	}

	if (!m_attackConfig)
	{
		Debug::error("[SkeletonScimitarState] SkeletonAttackConfig not found.");
		return;
	}

	if (!m_attackExecutor)
	{
		Debug::error("[SkeletonScimitarState] EnemyAttackExecutor not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SkeletonScimitarState] AnimationComponent not found.");
		return;
	}

	m_controller->clearPath();
	m_controller->resetRepathTimer();
	m_controller->updateCurrentTarget();

	m_previousAnimationSpeed = AnimationAPI::getSpeedMultiplier(m_animation);
	AnimationAPI::setSpeedMultiplier(m_animation, m_attackConfig->m_attackAnimationSpeed);

	changePhase(Phase::Dash);

	Debug::log("[SkeletonScimitarState] ENTER");
}

void SkeletonScimitarState::OnStateUpdate()
{
	if (!m_controller || !m_attackConfig || !m_attackExecutor || !m_animation)
	{
		return;
	}

	// To Death State
	if (m_controller->trySendReviveTrigger(m_animation))
	{
		return;
	}

	m_controller->faceCurrentTarget();

	m_phaseTimer += Time::getDeltaTime();

	if (m_phase == Phase::Dash)
	{
		updateDash();
		return;
	}

	if (m_phase == Phase::Attack1 || m_phase == Phase::Attack2 || m_phase == Phase::Attack3)
	{
		updateAttack();
		return;
	}

	if (m_phase == Phase::Backstep)
	{
		updateBackstep();
		return;
	}

	if (m_phase == Phase::Finished)
	{
		goToChase();
		return;
	}
}

void SkeletonScimitarState::OnStateExit()
{
	if (m_animation)
	{
		AnimationAPI::clearOverrideClip(m_animation, 0.0f);
	}

	m_phaseTimer = 0.0f;
	m_hasAppliedHit = false;
	AnimationAPI::setSpeedMultiplier(m_animation, m_previousAnimationSpeed);

	Debug::log("[SkeletonScimitarState] EXIT");
}

void SkeletonScimitarState::changePhase(Phase phase)
{
	m_phase = phase;
	m_phaseTimer = 0.0f;
	m_hasAppliedHit = false;

	if (!m_animation || !m_attackConfig)
	{
		return;
	}

	if (phase == Phase::Attack1 || phase == Phase::Attack2 || phase == Phase::Attack3)
	{
		AnimationAPI::playOverrideClip(m_animation, "Skeleton_Attak", 0.05, false);
	}
}

void SkeletonScimitarState::updateDash()
{
	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	Vector3 forward = TransformAPI::getForward(ownerTransform);
	forward.y = 0.0f;

	moveInDirection(forward, m_attackConfig->m_scimitarDashSpeed);

	if (m_controller->isCurrentTargetInRange(m_attackConfig->m_scimitarDashStopRange) ||
		m_phaseTimer >= m_attackConfig->m_scimitarDashDuration)
	{
		changePhase(Phase::Attack1);
		return;
	}
}

void SkeletonScimitarState::updateAttack()
{
	// Last Attack can stun
	if (!m_hasAppliedHit && m_phaseTimer >= getScimitarAttackHitTime())
	{
		const bool shouldStun = m_phase == Phase::Attack3;
		applyHit(shouldStun);
		m_hasAppliedHit = true;
	}

	if (m_phaseTimer < getScimitarAttackClipDuration())
	{
		return;
	}

	if (m_phase == Phase::Attack1)
	{
		changePhase(Phase::Attack2);
		return;
	}

	if (m_phase == Phase::Attack2)
	{
		changePhase(Phase::Backstep);
		return;
	}

	if (m_phase == Phase::Attack3)
	{
		changePhase(Phase::Finished);
		return;
	}
}

void SkeletonScimitarState::updateBackstep()
{
	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	Vector3 backward = -TransformAPI::getForward(ownerTransform);
	backward.y = 0.0f;

	moveInDirection(backward, m_attackConfig->m_stepBackSpeed);

	if (m_phaseTimer >= m_attackConfig->m_stepBackDuration)
	{
		changePhase(Phase::Attack3);
		return;
	}
}

void SkeletonScimitarState::applyHit(bool shouldStun)
{
	Transform* currentTarget = m_controller->getCurrentTarget();
	if (!currentTarget)
	{
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	const Vector3 center = TransformAPI::getGlobalPosition(ownerTransform);
	const Vector3 forward = TransformAPI::getForward(ownerTransform);

	if (shouldStun)
	{
		m_attackExecutor->tryDamageAndStunSingleTargetInCone(
			currentTarget,
			center,
			forward,
			m_attackConfig->m_scimitarStunHitRange,
			m_attackConfig->m_scimitarHalfAngleDegrees,
			m_attackConfig->m_basicAttackDamage,
			m_attackConfig->m_scimitarStunDuration,
			"SkeletonScimitar"
		);
	}
	else
	{
		m_attackExecutor->tryDamageTargetInCone(
			currentTarget,
			center,
			forward,
			m_attackConfig->m_basicAttackRange,
			m_attackConfig->m_scimitarHalfAngleDegrees,
			m_attackConfig->m_basicAttackDamage,
			"SkeletonScimitar"
		);
	}
}

void SkeletonScimitarState::moveInDirection(const Vector3& direction, float speed)
{
	if (direction.LengthSquared() <= 0.0001f)
	{
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	Vector3 moveDirection = direction;
	moveDirection.y = 0.0f;
	moveDirection.Normalize();

	Vector3 currentPosition = TransformAPI::getGlobalPosition(ownerTransform);
	Vector3 desiredPosition = currentPosition + moveDirection * speed * Time::getDeltaTime();

	Vector3 nextPosition;
	if (NavigationAPI::moveAlongSurface(currentPosition, desiredPosition, nextPosition, m_controller->m_pathSearchExtents))
	{
		TransformAPI::setPosition(ownerTransform, nextPosition);
	}
}

void SkeletonScimitarState::goToChase()
{
	if (!m_animation)
	{
		return;
	}

	AnimationAPI::clearOverrideClip(m_animation, 0.0f);
	AnimationAPI::sendTrigger(m_animation, "ToChase");
}

float SkeletonScimitarState::getScimitarAttackClipDuration() const
{
	return m_attackConfig->m_attackClipDuration / m_attackConfig->m_attackAnimationSpeed;
}

float SkeletonScimitarState::getScimitarAttackHitTime() const
{
	return getScimitarAttackClipDuration() * m_attackConfig->m_attackHitTime;
}

IMPLEMENT_SCRIPT(SkeletonScimitarState)