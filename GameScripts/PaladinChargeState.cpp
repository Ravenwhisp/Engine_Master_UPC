#include "pch.h"
#include "PaladinChargeState.h"

#include "MeleeEnemyController.h"
#include "PaladinAttackConfig.h"
#include "PaladinSound.h"
#include "PaladinVFX.h"

PaladinChargeState::PaladinChargeState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void PaladinChargeState::OnStateEnter()
{
	m_paladinController = GameObjectAPI::findScript<MeleeEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_paladinVFX = GameObjectAPI::findScript<PaladinVFX>(getOwner());

	m_stateTimer = 0.0f;

	if (!m_paladinController)
	{
		Debug::error("[PaladinChargeState] MeleeEnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[PaladinChargeState] AnimationComponent not found.");
		return;
	}

	if (!m_paladinVFX)
	{
		Debug::warn("[PaladinChargeState] PaladinVFX not found.");
	}

	m_paladinController->clearPath();
	m_paladinController->resetRepathTimer();

	m_chargeDirection = m_paladinController->getChargeDirection();

	m_paladinSound = GameObjectAPI::findScript<PaladinSound>(getOwner());
	if (m_paladinSound)
	{
		m_paladinSound->playChargeStart();
		m_paladinSound->startChargeLoop();
	}
	
	if (m_paladinVFX)
	{
		m_paladinVFX->startChargeAttackEffect();
	}

	Debug::log("[PaladinChargeState] ENTER");
}

void PaladinChargeState::OnStateUpdate()
{
	if (!m_paladinController || !m_paladinController->m_attackConfig.get() || !m_animation)
	{
		stopChargeAttackEffect();
		return;

	}

	if (m_paladinController->trySendDeathTrigger(m_animation))
	{
		stopChargeAttackEffect();
		return;

	}

	if (m_paladinController->trySendStunTrigger(m_animation))
	{
		stopChargeAttackEffect();
		return;
	}

	if (m_paladinController->isForcedMovementActive())
	{
		cancelCharge();
		return;
	}

	m_stateTimer += Time::getDeltaTime();

	moveCharge();

	if (m_stateTimer >= m_paladinController->m_attackConfig.get()->m_chargeDuration || m_paladinController->isTargetInAttackRange())
	{
		finishCharge();
		return;
	}
}

void PaladinChargeState::OnStateExit()
{
	// Covers every exit path (finish, death, stun) so the loop never lingers.
	if (m_paladinSound)
	{
		m_paladinSound->stopChargeLoop();
	}

	Debug::log("[PaladinChargeState] EXIT");

	stopChargeAttackEffect();
}

void PaladinChargeState::moveCharge()
{
	if (!m_paladinController || m_paladinController->isForcedMovementActive())
	{
		return;
	}

	if (!m_paladinController->m_attackConfig)
	{
		return;
	}

	if (m_chargeDirection.LengthSquared() <= 0.0001f)
	{
		return;
	}

	Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());

	if (!ownerTransform)
	{
		return;
	}

	Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
	Vector3 desiredPosition = ownerPosition;

	desiredPosition += m_chargeDirection * m_paladinController->m_attackConfig.get()->m_chargeSpeed * Time::getDeltaTime();

	Vector3 nextPosition;

	if (NavigationAPI::moveAlongSurface(ownerPosition, desiredPosition, nextPosition, Vector3(5.0f, 5.0f, 5.0f)))
	{
		TransformAPI::setGlobalPosition(ownerTransform, nextPosition);
	}
}

void PaladinChargeState::finishCharge()
{
	if (!m_paladinController || !m_animation)
	{
		stopChargeAttackEffect();
		return;
	}
	
	stopChargeAttackEffect();

	// Charge impact only when the embestida actually reaches the target.
	if (m_paladinSound && m_paladinController->isTargetInAttackRange())
	{
		m_paladinSound->playChargeImpact();
	}

	m_paladinController->consumeChargeCooldown();

	AnimationAPI::sendTrigger(m_animation, "ToChase");

	Debug::log("[PaladinChargeState] Finished, Chase trigger sent");
}

void PaladinChargeState::cancelCharge()
{
	if (!m_paladinController || !m_animation)
	{
		stopChargeAttackEffect();
		return;
	}

	stopChargeAttackEffect();

	if (m_paladinSound)
	{
		m_paladinSound->stopChargeLoop();
	}

	m_paladinController->consumeChargeCooldown();

	AnimationAPI::sendTrigger(m_animation, "ToChase");

	Debug::log("[PaladinChargeState] Charge cancelled by forced movement.");
}

void PaladinChargeState::stopChargeAttackEffect()
{
	if (m_paladinVFX)
	{
		m_paladinVFX->stopChargeAttackEffect();
	}
}

IMPLEMENT_SCRIPT(PaladinChargeState)