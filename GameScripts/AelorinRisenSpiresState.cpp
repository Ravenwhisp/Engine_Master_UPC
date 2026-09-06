#include "pch.h"
#include "AelorinRisenSpiresState.h"

#include "AelorinAttackConfig.h"
#include "AelorinAttackExecutor.h"
#include "AelorinUI.h"

AelorinRisenSpiresState::AelorinRisenSpiresState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinRisenSpiresState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinRisenSpiresState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	// get scripts
	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_aelorinUI = GameObjectAPI::findScript<AelorinUI>(parentGameObject);

	// reset members
	m_activeAbility = AelorinAbility::None;
	m_stateTimer = 0.0f;
	m_firstPassExecuted = false;
	m_secondPassExecuted = false;
	m_completed = false;
	m_isFuryCast = false;

	if (!m_controller)
	{
		Debug::error("[AelorinRisenSpiresState] AelorinBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[AelorinRisenSpiresState] AnimationComponent not found.");
		return;
	}

	if (!m_aelorinUI)
	{
		Debug::error("[AelorinRisenSpiresState] AelorinUI not found.");
	}

	m_attackExecutor = m_controller->getAttackExecutor();

	if (!m_attackExecutor)
	{
		Debug::error("[AelorinRisenSpiresState] AelorinAttackExecutor not found.");
		return;
	}

	// consume ability
	m_activeAbility = m_controller->consumeRequestedAbility();
	if (m_activeAbility != AelorinAbility::RisenSpires)
	{
		Debug::warn("[AelorinRisenSpiresState] Unexpected requested ability!");
		return;
	}

	m_isFuryCast = m_controller->isFuryActive();
	if (m_isFuryCast)
	{
		m_controller->recordFuryCast();
	}

	if (!m_isFuryCast && m_aelorinUI)
	{
		const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
		if (config)
		{
			m_aelorinUI->showRisenSpiresUI(m_controller->getRisenSpiresPatternARoot(), config->m_risenSpiresRadius, config->m_risenSpiresWindupDuration);
		}
	}

	Debug::log("[AelorinRisenSpiresState] ENTER");
}

void AelorinRisenSpiresState::OnStateUpdate()
{
	if (!m_controller || !m_attackExecutor || !m_animation || m_completed)
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

	// Pick Fury type cast or Normal
	const float windupDuration = m_isFuryCast ? 0.0f : config->m_risenSpiresWindupDuration;
	const float recoveryDuration = m_isFuryCast ? 0.0f : config->m_risenSpiresRecoveryDuration;

	// First pass after the 3 second windup
	if (!m_firstPassExecuted && m_stateTimer >= windupDuration)
	{
		executePattern(m_controller->getRisenSpiresPatternARoot(), "Risen Spires Pass 1");
		m_firstPassExecuted = true;

		// UI Phase 2 reveal pattern B
		if (!m_isFuryCast && m_controller->isPhase2() && m_aelorinUI)
		{
			m_aelorinUI->showRisenSpiresUI(m_controller->getRisenSpiresPatternBRoot(), config->m_risenSpiresRadius, config->m_risenSpiresPhase2SecondPassDelay);
		}
	}

	// Phase 2 gets opposite pattern 2 seconds later
	if (m_controller->isPhase2() && m_firstPassExecuted && !m_secondPassExecuted && m_stateTimer >= windupDuration + config->m_risenSpiresPhase2SecondPassDelay)
	{
		executePattern(m_controller->getRisenSpiresPatternBRoot(), "Risen Spires Pass 2");
		m_secondPassExecuted = true;
	}

	const float lastPassTime = m_controller->isPhase2() ? windupDuration + config->m_risenSpiresPhase2SecondPassDelay : windupDuration;

	if (m_stateTimer < lastPassTime + recoveryDuration)
	{
		return;
	}

	finishAbility();
}

void AelorinRisenSpiresState::OnStateExit()
{
	if (m_aelorinUI)
	{
		m_aelorinUI->cancelRisenSpires();
	}

	m_aelorinUI = nullptr;
	m_stateTimer = 0.0f;
	m_firstPassExecuted = false;
	m_secondPassExecuted = false;
	m_completed = false;
	m_isFuryCast = false;

	Debug::log("[AelorinRisenSpiresState] EXIT");
}

void AelorinRisenSpiresState::executePattern(Transform* patternRoot, const char* sourceName)
{
	if (!patternRoot || !m_attackExecutor)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	// pattern has child game objects and uses their transform to position the attack
	const int childCount = TransformAPI::getChildCount(patternRoot);

	for (int i = 0; i < childCount; ++i)
	{
		Transform* spirePoint = TransformAPI::getChild(patternRoot, i);
		if (!spirePoint)
		{
			continue;
		}

		const Vector3 position = TransformAPI::getGlobalPosition(spirePoint);

		m_attackExecutor->applyDamageInRadius(position, config->m_risenSpiresRadius, config->m_risenSpiresDamage, sourceName);
	}

	Debug::log("[AelorinRisenSpiresState] Executed pattern with %d spires", childCount);
}

void AelorinRisenSpiresState::finishAbility()
{
	if (m_completed)
	{
		return;
	}

	m_completed = true;

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToIdle");
	if (!sent)
	{
		Debug::warn("[AelorinRisenSpiresState] Failed to send ToIdle trigger");
	}
}

IMPLEMENT_SCRIPT(AelorinRisenSpiresState)