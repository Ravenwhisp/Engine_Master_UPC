#include "pch.h"
#include "PaladinChargeState.h"

#include "MeleeEnemyController.h"
#include "PaladinAttackConfig.h"

PaladinChargeState::PaladinChargeState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void PaladinChargeState::OnStateEnter()
{
	m_paladinController = GameObjectAPI::findScript<MeleeEnemyController>(getOwner());
	m_attackConfig = GameObjectAPI::findScript<PaladinAttackConfig>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	m_stateTimer = 0.0f;

	if (!m_paladinController)
	{
		Debug::error("[PaladinChargeState] MeleeEnemyController not found.");
		return;
	}

	if (!m_attackConfig)
	{
		Debug::error("[PaladinChargeState] PaladinAttackConfig not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[PaladinChargeState] AnimationComponent not found.");
		return;
	}

	m_paladinController->clearPath();
	m_paladinController->resetRepathTimer();

	m_chargeDirection = m_paladinController->getChargeDirection();

	Debug::log("[PaladinChargeState] ENTER");
}

void PaladinChargeState::OnStateUpdate()
{
	if (!m_paladinController || !m_attackConfig || !m_animation)
	{
		return;
	}

	if (m_paladinController->trySendDeathTrigger(m_animation))
	{
		return;
	}

	if (m_paladinController->trySendStunTrigger(m_animation))
	{
		return;
	}

	m_stateTimer += Time::getDeltaTime();

	moveCharge();

	if (m_stateTimer >= m_attackConfig->m_chargeDuration || m_paladinController->isTargetInAttackRange())
	{
		finishCharge();
		return;
	}
}

void PaladinChargeState::OnStateExit()
{
	Debug::log("[PaladinChargeState] EXIT");
}

void PaladinChargeState::moveCharge()
{
	if (!m_attackConfig)
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

	desiredPosition += m_chargeDirection * m_attackConfig->m_chargeSpeed * Time::getDeltaTime();

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
		return;
	}

	m_paladinController->consumeChargeCooldown();

	AnimationAPI::sendTrigger(m_animation, "ToChase");

	Debug::log("[PaladinChargeState] Finished, Chase trigger sent");
}

IMPLEMENT_SCRIPT(PaladinChargeState)