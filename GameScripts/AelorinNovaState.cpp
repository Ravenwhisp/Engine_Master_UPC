#include "pch.h"
#include "AelorinNovaState.h"
#include "AelorinAttackExecutor.h"
#include "AelorinUI.h"

#include "AelorinAttackConfig.h"

AelorinNovaState::AelorinNovaState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinNovaState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinNovaState] Aelorin transform not found.");
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
	m_firstWaveApplied = false;
	m_secondWaveApplied = false;
	m_completed = false;
	m_isFuryCast = false;

	if (!m_controller)
	{
		Debug::error("[AelorinNovaState] AelorinBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[AelorinNovaState] AnimationComponent not found.");
		return;
	}

	if (!m_aelorinUI)
	{
		Debug::error("[AelorinNovaState] AelorinUI not found.");
	}

	// consume ability
	m_activeAbility = m_controller->consumeRequestedAbility();
	if (m_activeAbility != AelorinAbility::Nova)
	{
		Debug::warn("[AelorinNovaState] Unexpected requested ability!");
		return;
	}

	m_isFuryCast = m_controller->isFuryActive();

	if (m_controller->hasNovaCenterOverride())
	{
		m_novaCenter = m_controller->consumeNovaCenterOverride();
	}
	else
	{
		m_novaCenter = TransformAPI::getGlobalPosition(parentTransform);
	}

	if (!m_isFuryCast && m_aelorinUI)
	{
		const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
		if (config)
		{
			m_aelorinUI->showNovaUI(
				m_novaCenter,
				config->m_novaRadius,
				config->m_novaChargeTime,
				m_controller->isPhase2(),
				config->m_novaPhase2SecondRadius,
				config->m_novaPhase2SecondWaveDelay
			);
		}
	}

	Debug::log("[AelorinNovaState] ENTER");
}

void AelorinNovaState::OnStateUpdate()
{
	if (!m_controller || !m_animation ||  m_completed)
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

	// Fury strips the nova windup + recovery
	const float chargeTime = m_isFuryCast ? 0.0f : config->m_novaChargeTime;
	const float recoveryDuration = m_isFuryCast ? 0.0f : config->m_novaRecoveryDuration;

	if (!m_firstWaveApplied && m_stateTimer >= chargeTime)
	{
		executeFirstNovaWave();
		m_firstWaveApplied = true;
	}

	if (m_controller->isPhase2() &&
		m_firstWaveApplied &&
		!m_secondWaveApplied &&
		m_stateTimer >= chargeTime + config->m_novaPhase2SecondWaveDelay)
	{
		executeSecondNovaWave();
		m_secondWaveApplied = true;
	}

	const float lastWaveTime = m_controller->isPhase2() ? chargeTime + config->m_novaPhase2SecondWaveDelay : chargeTime;

	if (m_stateTimer < lastWaveTime + recoveryDuration)
	{
		return;
	}

	finishAbility();
}

void AelorinNovaState::OnStateExit()
{
	if (m_aelorinUI)
	{
		m_aelorinUI->cancelNova();
	}

	m_aelorinUI = nullptr;
	m_stateTimer = 0.0f;
	m_novaCenter = Vector3::Zero;
	m_firstWaveApplied = false;
	m_secondWaveApplied = false;
	m_completed = false;
	m_isFuryCast = false;

	Debug::log("[AelorinNovaState] EXIT");
}

void AelorinNovaState::executeFirstNovaWave()
{
	if (!m_controller)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	executeNovaWave(config->m_novaRadius, config->m_novaDamage);

	Debug::log("[AelorinNovaState] First Nova wave.");
}

void AelorinNovaState::executeSecondNovaWave()
{
	if (!m_controller)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	executeNovaWave(config->m_novaPhase2SecondRadius, config->m_novaPhase2SecondDamage);

	Debug::log("[AelorinNovaState] Second Nova wave.");
}

void AelorinNovaState::executeNovaWave(float radius, float damage)
{
	AelorinAttackExecutor* executor = m_controller->getAttackExecutor();

	if (!executor)
	{
		return;
	}

	executor->applyDamageInRadius(m_novaCenter, radius, damage, "Aelorin Nova");
}

void AelorinNovaState::finishAbility()
{
	if (m_completed)
	{
		return;
	}

	m_completed = true;

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToIdle");
	if (!sent)
	{
		Debug::warn("[AelorinNovaState] Failed to send ToIdle trigger");
	}
}

IMPLEMENT_SCRIPT(AelorinNovaState)