#include "pch.h"
#include "AelorinExhaustionState.h"

#include "AelorinBossController.h"
#include "AelorinAttackConfig.h"

AelorinExhaustionState::AelorinExhaustionState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinExhaustionState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinExhaustionState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	// get scripts
	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	// reset members
	m_stateTimer = 0.0f;
	m_completed = false;

	if (!m_controller)
	{
		Debug::error("[AelorinExhaustionState] AelorinBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[AelorinExhaustionState] AnimationComponent not found.");
		return;
	}

	Debug::log("[AelorinExhaustionState] ENTER");
}

void AelorinExhaustionState::OnStateUpdate()
{
	if (!m_controller || !m_animation || m_completed)
	{
		return;
	}

	if (m_controller->trySendPriorityInterrupt(m_animation))
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	m_stateTimer += Time::getDeltaTime();

	if (m_stateTimer < config->m_furyExhaustionDuration)
	{
		return;
	}

	finishExhaustion();
}

void AelorinExhaustionState::OnStateExit()
{
	m_stateTimer = 0.0f;
	m_completed = false;

	Debug::log("[AelorinExhaustionState] EXIT");
}

void AelorinExhaustionState::finishExhaustion()
{
	if (m_completed)
	{
		return;
	}

	m_completed = true;

	// Fury officialy ends after vulnerability window here
	m_controller->finishFury();

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToIdle");
	if (!sent)
	{
		Debug::warn("[AelorinExhaustionState] Failed to send ToIdle trigger");
	}
}

IMPLEMENT_SCRIPT(AelorinExhaustionState)