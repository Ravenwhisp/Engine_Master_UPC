#include "pch.h"
#include "AelorinIdleState.h"

#include "AelorinBossController.h"

AelorinIdleState::AelorinIdleState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinIdleState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinIdleState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	m_decisionTimer = 0.0f;
	m_decisionMade = false;

	if (!m_controller)
	{
		Debug::error("[AelorinIdleState] AelorinBossController not found.");
	}

	if (!m_animation)
	{
		Debug::error("[AelorinIdleState] AnimationComponent not found.");
	}

	Debug::log("[AelorinIdleState] ENTER");
}

void AelorinIdleState::OnStateUpdate()
{
	if (!m_controller || !m_animation)
	{
		return;
	}

	if (!m_controller->hasEncounterStarted())
	{
		return;
	}

	if (m_controller->trySendPriorityInterrupt(m_animation))
	{
		return;
	}
	
	// FURY
	if (m_controller->isFuryRequested())
	{
		m_controller->beginFury();
	}

	if (m_controller->isFuryActive())
	{
		if (m_controller->isFuryBarrageComplete())
		{
			m_controller->trySendSoulCataclysmTrigger(m_animation);
			return;
		}

		const AelorinAbility furyAbility = m_controller->chooseNextFuryAbility();
		if (furyAbility == AelorinAbility::None)
		{
			Debug::warn("[AelorinIdleState] No valid Fury ability");
			return;
		}

		if (!m_controller->requestAbility(furyAbility))
		{
			Debug::warn("[AelorinIdleState] Failed to request Fury ability.");
			return;
		}

		if (!m_controller->trySendRequestedAbilityTrigger(m_animation))
		{
			Debug::warn("[AelorinIdleState] Failed to send Fury ability trigger.");

			m_controller->clearRequestedAbility();
			return;
		}

		return;
	}

	// NORMAL
	if (m_decisionMade)
	{
		return;
	}

	m_decisionTimer += Time::getDeltaTime();

	if (m_decisionTimer < m_controller->getDecisionTime())
	{
		return;
	}

	m_decisionMade = true;

	const AelorinAbility selectedAbility = m_controller->chooseNextAbility();
	if (selectedAbility == AelorinAbility::None)
	{
		Debug::warn("[AelorinIdleState] No valid ability available.");

		m_decisionMade = false;
		m_decisionTimer = 0.0f;
		return;
	}

	if (!m_controller->requestAbility(selectedAbility))
	{
		Debug::warn("[AelorinIdleState] Failed to request selected ability.");

		m_decisionMade = false;
		m_decisionTimer = 0.0f;
		return;
	}

	if (!m_controller->trySendRequestedAbilityTrigger(m_animation))
	{
		Debug::warn("[AelorinIdleState] Failed to send ability trigger.");

		m_controller->clearRequestedAbility();
		m_decisionMade = false;
		m_decisionTimer = 0.0f;
		return;
	}

	Debug::log("[AelorinIdleState] Selected ability: %d", static_cast<int>(selectedAbility));
}

void AelorinIdleState::OnStateExit()
{
	Debug::log("[AelorinIdleState] EXIT");
}

IMPLEMENT_SCRIPT(AelorinIdleState)