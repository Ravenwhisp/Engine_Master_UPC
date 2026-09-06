#include "pch.h"
#include "SkeletonScimitarState.h"

#include "SkeletonEnemyController.h"
#include "SkeletonAttackConfig.h"
#include "EnemyAttackExecutor.h"
#include "SkeletonParticles.h"
#include "SkeletonUI.h"

SkeletonScimitarState::SkeletonScimitarState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SkeletonScimitarState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<SkeletonEnemyController>(getOwner());
	m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_particles = GameObjectAPI::findScript<SkeletonParticles>(getOwner());
	m_skeletonUI = GameObjectAPI::findScript<SkeletonUI>(getOwner());

	if (!m_controller)
	{
		Debug::error("[SkeletonScimitarState] SkeletonEnemyController not found.");
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

	if (!m_particles)
	{
		Debug::warn("[SkeletonScimitarState] SkeletonParticles not found.");
	}

	if (!m_skeletonUI)
	{
		Debug::warn("[SkeletonScimitarState] SkeletonUI not found.");
	}

	m_controller->clearPath();
	m_controller->resetRepathTimer();
	m_controller->updateCurrentTarget();

	m_previousAnimationSpeed = AnimationAPI::getSpeedMultiplier(m_animation);
	AnimationAPI::setSpeedMultiplier(m_animation, m_controller->m_attackConfig.get()->m_attackAnimationSpeed);

	if (m_skeletonUI)
	{
		m_skeletonUI->hideScimitarUI();
	}

	changePhase(Phase::Dash);

	Debug::log("[SkeletonScimitarState] ENTER");
}

void SkeletonScimitarState::OnStateUpdate()
{
	if (!m_controller || !m_controller->m_attackConfig.get() || !m_attackExecutor || !m_animation)
	{
		return;
	}

	// To Death State
	if (m_controller->trySendReviveTrigger(m_animation))
	{
		return;
	}

	m_phaseTimer += Time::getDeltaTime();

	if (m_phase == Phase::Dash)
	{
		m_controller->faceCurrentTarget();
		updateDash();
		return;
	}

	if (m_phase == Phase::Attack1 || m_phase == Phase::Attack2 || m_phase == Phase::Attack3)
	{
		updateAttack();
		return;
	}

	if (m_phase == Phase::Reaim1 || m_phase == Phase::Reaim2)
	{
		m_controller->faceCurrentTarget();

		if (m_phaseTimer >= m_reaimDuration)
		{
			if (m_phase == Phase::Reaim1)
			{
				changePhase(Phase::Attack2);
			}
			else
			{
				changePhase(Phase::Attack3);
			}
		}

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
		AnimationAPI::setSpeedMultiplier(m_animation, m_previousAnimationSpeed);
	}

	if (m_skeletonUI)
	{
		m_skeletonUI->hideScimitarUI();
	}

	m_phaseTimer = 0.0f;
	m_hasAppliedHit = false;

	Debug::log("[SkeletonScimitarState] EXIT");
}

void SkeletonScimitarState::changePhase(Phase phase)
{
	m_phase = phase;
	m_phaseTimer = 0.0f;
	m_hasAppliedHit = false;

	if (!m_animation || !m_controller->m_attackConfig.get())
	{
		return;
	}

	if (m_skeletonUI)
	{
		m_skeletonUI->hideScimitarUI();
	}

	if (phase == Phase::Attack1 || phase == Phase::Attack2 || phase == Phase::Attack3)
	{
		AnimationAPI::playOverrideClip(m_animation, "Skeleton_Attak", 0.05, false);
		setupAttackTelegraph();
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

	moveInDirection(forward, m_controller->m_attackConfig.get()->m_scimitarDashSpeed);

	if (m_controller->isCurrentTargetInRange(m_controller->m_attackConfig.get()->m_scimitarDashStopRange) ||
		m_phaseTimer >= m_controller->m_attackConfig.get()->m_scimitarDashDuration)
	{
		changePhase(Phase::Attack1);
		return;
	}
}

void SkeletonScimitarState::updateAttack()
{
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
		changePhase(Phase::Reaim1);
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

	moveInDirection(backward, m_controller->m_attackConfig.get()->m_stepBackSpeed);

	if (m_phaseTimer >= m_controller->m_attackConfig.get()->m_stepBackDuration)
	{
		changePhase(Phase::Reaim2);
		return;
	}
}

void SkeletonScimitarState::setupAttackTelegraph()
{
	if (!m_skeletonUI || !m_controller || !m_controller->m_attackConfig.get())
	{
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	const Vector3 origin = TransformAPI::getGlobalPosition(ownerTransform);
	const Vector3 forward = TransformAPI::getForward(ownerTransform);

	const float range =
		m_phase == Phase::Attack3
		? m_controller->m_attackConfig.get()->m_scimitarStunHitRange
		: m_controller->m_attackConfig.get()->m_basicAttackRange;

	m_skeletonUI->setupScimitarUI(
		range,
		m_controller->m_attackConfig.get()->m_scimitarHalfAngleDegrees
	);

	m_skeletonUI->showScimitarTelegraph(
		origin,
		forward
	);
}

void SkeletonScimitarState::applyHit(bool shouldStun)
{
	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
	if (!ownerTransform)
	{
		return;
	}

	const Vector3 center = TransformAPI::getGlobalPosition(ownerTransform);
	const Vector3 forward = TransformAPI::getForward(ownerTransform);

	if (m_skeletonUI)
	{
		m_skeletonUI->showScimitarImpact();
	}

	if (m_particles && m_attackExecutor)
	{
		if (shouldStun)
		{
			m_attackExecutor->setNextPlayerHitVfx(m_particles->getThirdAttackHitVfxId());
		}
		else
		{
			m_attackExecutor->setNextPlayerHitVfx(m_particles->getShieldHitVfxId());
		}
	}

	if (shouldStun)
	{
		m_attackExecutor->applyDamageAndStunInCone(
			center,
			forward,
			m_controller->m_attackConfig.get()->m_scimitarStunHitRange,
			m_controller->m_attackConfig.get()->m_scimitarHalfAngleDegrees,
			m_controller->m_attackConfig.get()->m_basicAttackDamage,
			m_controller->m_attackConfig.get()->m_scimitarStunDuration,
			"SkeletonScimitar"
		);
	}
	else
	{
		m_attackExecutor->applyDamageInCone(
			center,
			forward,
			m_controller->m_attackConfig.get()->m_basicAttackRange,
			m_controller->m_attackConfig.get()->m_scimitarHalfAngleDegrees,
			m_controller->m_attackConfig.get()->m_basicAttackDamage,
			"SkeletonScimitar"
		);
	}
}

void SkeletonScimitarState::moveInDirection(const Vector3& direction, float speed)
{
	if (!m_controller || m_controller->isForcedMovementActive())
	{
		return;
	}

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
		TransformAPI::setGlobalPosition(ownerTransform, nextPosition);
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
	return m_controller->m_attackConfig.get()->m_attackClipDuration / m_controller->m_attackConfig.get()->m_attackAnimationSpeed;
}

float SkeletonScimitarState::getScimitarAttackHitTime() const
{
	return getScimitarAttackClipDuration() * m_controller->m_attackConfig.get()->m_attackHitTime;
}

IMPLEMENT_SCRIPT(SkeletonScimitarState)