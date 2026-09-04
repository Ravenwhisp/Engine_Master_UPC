#include "pch.h"
#include "AelorinTeleportState.h"

#include "AelorinAttackConfig.h"
#include "AelorinAttackExecutor.h"

AelorinTeleportState::AelorinTeleportState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinTeleportState::OnStateEnter()
{
	m_aelorinTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!m_aelorinTransform)
	{
		Debug::error("[AelorinTeleportState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(m_aelorinTransform);

	// get scripts
	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	// reset members
	m_crowdingPlayer = nullptr;
	m_activeAbility = AelorinAbility::None;
	m_stateTimer = 0.0f;
	m_recoveryTimer = 0.0f;
	m_teleportExecuted = false;
	m_completed = false;

	if (!m_controller)
	{
		Debug::error("[AelorinTeleportState] AelorinBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[AelorinTeleportState] AnimationComponent not found.");
		return;
	}

	m_attackExecutor = m_controller->getAttackExecutor();

	if (!m_attackExecutor)
	{
		Debug::error("[AelorinTeleportState] AelorinAttackExecutor not found.");
		return;
	}

	// consume ability
	m_activeAbility = m_controller->consumeRequestedAbility();
	if (m_activeAbility != AelorinAbility::Teleport)
	{
		Debug::warn("[AelorinTeleportState] Unexpected requested ability!");
		return;
	}

	// lock which player caused the defensive teleport
	m_crowdingPlayer = m_controller->getTeleportCrowdingPlayer();
	if (!m_crowdingPlayer)
	{
		Debug::warn("[AelorinTeleportState] No crowding player found.");
	}	

	Debug::log("[AelorinTeleportState] ENTER");
}

void AelorinTeleportState::OnStateUpdate()
{
	if (!m_controller || !m_animation || !m_aelorinTransform || m_completed)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	if (!m_teleportExecuted)
	{
		m_stateTimer += Time::getDeltaTime();

		if (m_stateTimer < config->m_teleportCastDuration)
		{
			return;
		}

		executeTeleport();
		m_teleportExecuted = true;
		return;
	}

	m_recoveryTimer += Time::getDeltaTime();
	
	if (m_recoveryTimer < config->m_teleportRecoveryDuration)
	{
		return;
	}

	finishAbility();
}

void AelorinTeleportState::OnStateExit()
{
	m_crowdingPlayer = nullptr;
	m_aelorinTransform = nullptr;
	m_stateTimer = 0.0f;
	m_recoveryTimer = 0.0f;
	m_teleportExecuted = false;
	m_completed = false;

	Debug::log("[AelorinTeleportState] EXIT");
}

void AelorinTeleportState::executeTeleport()
{
	if (!m_controller || !m_aelorinTransform)
	{
		return;
	}

	if (!m_crowdingPlayer)
	{
		Debug::warn("[AelorinTeleportState] Teleport cancelled: crowding player is missing.");
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	// pick destination at resolve time -> player may have moved during the 3 second cast
	Transform* destinationAnchor = m_controller->chooseTeleportAnchor(m_crowdingPlayer);
	if (!destinationAnchor)
	{
		Debug::warn("[AelorinTeleportState] Teleport cancelled: no valid anchor.");
		return;
	}

	const Vector3 departurePosition = TransformAPI::getGlobalPosition(m_aelorinTransform);

	// Phase 2 damage burst
	if (m_controller->isPhase2() && m_attackExecutor)
	{
		m_attackExecutor->applyDamageInRadius(departurePosition, config->m_teleportPhase2BurstRadius, config->m_teleportPhase2BurstDamage, "Aelorin Teleport - Departure Burst");
	}

	const Vector3 destinationPosition = TransformAPI::getGlobalPosition(destinationAnchor);
	TransformAPI::setGlobalPosition(m_aelorinTransform, destinationPosition);

	// start cooldown
	m_controller->startTeleportCooldown();
}

void AelorinTeleportState::finishAbility()
{
	if (m_completed)
	{
		return;
	}

	m_completed = true;

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToIdle");
	if (!sent)
	{
		Debug::warn("[AelorinTeleportState] Failed to send ToIdle trigger");
	}
}

IMPLEMENT_SCRIPT(AelorinTeleportState)