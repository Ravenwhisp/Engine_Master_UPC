#include "pch.h"
#include "AelorinSpiritCannonState.h"

#include "AelorinAttackConfig.h"
#include "AelorinAttackExecutor.h"
#include "AelorinUI.h"

#include <cstdlib>

AelorinSpiritCannonState::AelorinSpiritCannonState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinSpiritCannonState::OnStateEnter()
{
	m_aelorinTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!m_aelorinTransform)
	{
		Debug::error("[AelorinSpiritCannonState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(m_aelorinTransform);

	// get scripts
	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_aelorinUI = GameObjectAPI::findScript<AelorinUI>(parentGameObject);

	// reset members
	m_lockedTarget = nullptr;
	m_activeAbility = AelorinAbility::None;
	m_stateTimer = 0.0f;
	m_shotCount = 0;
	m_completed = false;
	m_isFuryCast = false;

	if (!m_controller)
	{
		Debug::error("[AelorinSpiritCannonState] AelorinBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[AelorinSpiritCannonState] AnimationComponent not found.");
		return;
	}

	if (!m_aelorinUI)
	{
		Debug::error("[AelorinSpiritCannonState] AelorinUI not found.");
	}

	m_attackExecutor = m_controller->getAttackExecutor();

	if (!m_attackExecutor)
	{
		Debug::error("[AelorinSpiritCannonState] AelorinAttackExecutor not found.");
		return;
	}

	// consume ability
	m_activeAbility = m_controller->consumeRequestedAbility();
	if (m_activeAbility != AelorinAbility::SpiritCannon)
	{
		Debug::warn("[AelorinSpiritCannonState] Unexpected requested ability!");
		return;
	}

	selectLockedTarget();
	if (!m_lockedTarget)
	{
		Debug::warn("[AelorinSpiritCannonState] No valid target found.");
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
			m_aelorinUI->showSpiritCannonUI(
				m_aelorinTransform,
				m_lockedTarget,
				config->m_spiritCannonBeamLength,
				config->m_spiritCannonBeamWidth,
				config->m_spiritCannonWindupDuration);
		}
	}

	Debug::log("[AelorinSpiritCannonState] ENTER");
}

void AelorinSpiritCannonState::OnStateUpdate()
{
	if (!m_controller || !m_attackExecutor || !m_animation || !m_aelorinTransform || m_completed)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	m_stateTimer += Time::getDeltaTime();

	// Pick Fury or Normal delay
	const float windupDuration = m_isFuryCast ? 0.0f : config->m_spiritCannonWindupDuration;
	const float recoveryDuration = m_isFuryCast ? 0.0f : config->m_spiritCannonRecoveryDuration;

	// PHASE 1
	// Windup -> Shot 1 -> Re-aim -> Shot 2 -> Recovery
	if (!m_controller->isPhase2())
	{
		const float shot1Time = windupDuration;
		const float shot2Time = shot1Time + config->m_spiritCannonPhase1ShotInterval;

		if (m_shotCount == 0 && m_stateTimer >= shot1Time)
		{
			ensureValidLockedTarget();
			fireBeamShot(config->m_spiritCannonBeamWidth, config->m_spiritCannonDamage, "Aelorin Spirit Cannon Shot 1");
			++m_shotCount;

			// UI
			if (!m_isFuryCast && m_aelorinUI && m_lockedTarget)
			{
				m_aelorinUI->showSpiritCannonUI(
					m_aelorinTransform,
					m_lockedTarget,
					config->m_spiritCannonBeamLength,
					config->m_spiritCannonBeamWidth,
					config->m_spiritCannonPhase1ShotInterval);
			}
			return;
		}

		if (m_shotCount == 1 && m_stateTimer >= shot2Time)
		{
			ensureValidLockedTarget();
			fireBeamShot(config->m_spiritCannonBeamWidth, config->m_spiritCannonDamage, "Aelorin Spirit Cannon Shot 2");
			++m_shotCount;
			return;
		}

		if (m_shotCount >= 2 && m_stateTimer >= shot2Time + recoveryDuration)
		{
			finishAbility();
		}

		return;
	}

	// PHASE 2
	// Windup -> Quick Shot 1 -> Quick Shot 2 -> Quick Shot 3 -> Final Slow Wide Shot -> Recovery

	const float shot1Time = windupDuration;
	const float shot2Time = shot1Time + config->m_spiritCannonPhase2ShotInterval;
	const float shot3Time = shot2Time + config->m_spiritCannonPhase2ShotInterval;
	const float finalShotTime = shot3Time + config->m_spiritCannonPhase2FinalShotDelay;
	
	if (m_shotCount == 0 && m_stateTimer >= shot1Time)
	{
		ensureValidLockedTarget();
		fireBeamShot(config->m_spiritCannonBeamWidth, config->m_spiritCannonDamage, "Aelorin Spirit Cannon Quick Shot 1");
		++m_shotCount;

		// UI
		if (!m_isFuryCast && m_aelorinUI && m_lockedTarget)
		{
			m_aelorinUI->showSpiritCannonUI(
				m_aelorinTransform,
				m_lockedTarget,
				config->m_spiritCannonBeamLength,
				config->m_spiritCannonBeamWidth,
				config->m_spiritCannonPhase2ShotInterval);
		}

		return;
	}

	if (m_shotCount == 1 && m_stateTimer >= shot2Time)
	{
		ensureValidLockedTarget();
		fireBeamShot(config->m_spiritCannonBeamWidth, config->m_spiritCannonDamage, "Aelorin Spirit Cannon Quick Shot 2");
		++m_shotCount;

		// UI
		if (!m_isFuryCast && m_aelorinUI && m_lockedTarget)
		{
			m_aelorinUI->showSpiritCannonUI(
				m_aelorinTransform,
				m_lockedTarget,
				config->m_spiritCannonBeamLength,
				config->m_spiritCannonBeamWidth,
				config->m_spiritCannonPhase2ShotInterval);
		}

		return;
	}

	if (m_shotCount == 2 && m_stateTimer >= shot3Time)
	{
		ensureValidLockedTarget();
		fireBeamShot(config->m_spiritCannonBeamWidth, config->m_spiritCannonDamage, "Aelorin Spirit Cannon Quick Shot 3");
		++m_shotCount;

		// UI
		if (!m_isFuryCast && m_aelorinUI && m_lockedTarget)
		{
			m_aelorinUI->showSpiritCannonUI(
				m_aelorinTransform,
				m_lockedTarget,
				config->m_spiritCannonBeamLength,
				config->m_spiritCannonPhase2FinalBeamWidth,
				config->m_spiritCannonPhase2FinalShotDelay);
		}

		return;
	}

	if (m_shotCount == 3 && m_stateTimer >= finalShotTime)
	{
		ensureValidLockedTarget();
		fireBeamShot(config->m_spiritCannonPhase2FinalBeamWidth, config->m_spiritCannonPhase2FinalDamage, "Aelorin Spirit Cannon Final Shot");
		++m_shotCount;
		return;
	}

	if (m_shotCount >= 4 && m_stateTimer >= finalShotTime + recoveryDuration)
	{
		finishAbility();
	}
}

void AelorinSpiritCannonState::OnStateExit()
{
	m_aelorinUI = nullptr;
	m_lockedTarget = nullptr;
	m_aelorinTransform = nullptr;

	m_stateTimer = 0.0f;
	m_shotCount = 0;
	m_completed = false;
	m_isFuryCast = false;

	if (m_controller)
	{
		m_controller->clearSpiritCannonDebugLine();
	}

	Debug::log("[AelorinSpiritCannonState] EXIT");
}

void AelorinSpiritCannonState::selectLockedTarget()
{
	if (!m_controller)
	{
		return;
	}

	Transform* lyriel = m_controller->getLyrielTransform();
	Transform* death = m_controller->getDeathTransform();

	const bool lyrielValid = isValidTarget(lyriel);
	const bool deathValid = isValidTarget(death);

	if (lyrielValid && !deathValid)
	{
		m_lockedTarget = lyriel;
		return;
	}

	if (!lyrielValid && deathValid)
	{
		m_lockedTarget = death;
		return;
	}

	if (!lyrielValid && !deathValid)
	{
		m_lockedTarget = nullptr;
		return;
	}

	// select a player at random since design does not specify which one
	m_lockedTarget = (std::rand() % 2 == 0) ? lyriel : death;
}

bool AelorinSpiritCannonState::isValidTarget(Transform* targetTransform) const
{
	return m_attackExecutor && m_attackExecutor->isValidDamageTarget(targetTransform);
}

void AelorinSpiritCannonState::ensureValidLockedTarget()
{
	if (isValidTarget(m_lockedTarget))
	{
		return;
	}

	selectLockedTarget();
}

void AelorinSpiritCannonState::fireBeamShot(float width, float damage, const char* sourceName)
{
	if (!m_lockedTarget || !m_aelorinTransform || !m_attackExecutor || !m_controller)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	const Vector3 origin = TransformAPI::getGlobalPosition(m_aelorinTransform);
	const Vector3 targetPosition = TransformAPI::getGlobalPosition(m_lockedTarget);

	Vector3 direction = targetPosition - origin;
	direction.y = 0.0f;

	if (direction.LengthSquared() < 0.00001f)
	{
		return;
	}

	direction.Normalize();

	m_controller->setSpiritCannonDebugLine(origin, direction, width);

	m_attackExecutor->applyDamageInBeam(origin, direction, config->m_spiritCannonBeamLength, width, damage, sourceName);
}

void AelorinSpiritCannonState::finishAbility()
{
	if (m_completed)
	{
		return;
	}

	m_completed = true;

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToIdle");
	if (!sent)
	{
		Debug::warn("[AelorinSpiritCannonState] Failed to send ToIdle trigger.");
	}
}

IMPLEMENT_SCRIPT(AelorinSpiritCannonState)