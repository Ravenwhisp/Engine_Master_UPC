#include "pch.h"
#include "AelorinSpiritCannonState.h"

#include "AelorinAttackConfig.h"
#include "AelorinAttackExecutor.h"
#include "AelorinUI.h"

#include <cstdlib>
#include <algorithm>
#include <cmath>

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
	m_currentAimDirection = Vector3::Zero;
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

	initializeAimDirection();

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
				m_currentAimDirection,
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

	const bool isPhase2 = m_controller->isPhase2();

	// Timings
	const float windupDuration = m_isFuryCast ? 0.0f : config->m_spiritCannonWindupDuration;
	const float recoveryDuration = m_isFuryCast ? 0.0f : config->m_spiritCannonRecoveryDuration;

	//			Phase 1
	const float phase1Shot1Time = windupDuration;
	const float phase1Shot2Time = phase1Shot1Time + config->m_spiritCannonPhase1ShotInterval;

	//			Phase 2
	const float phase2Shot1Time = windupDuration;
	const float phase2Shot2Time = phase2Shot1Time + config->m_spiritCannonPhase2ShotInterval;
	const float phase2Shot3Time = phase2Shot2Time + config->m_spiritCannonPhase2ShotInterval;
	const float phase2FinalShotTime = phase2Shot3Time + config->m_spiritCannonPhase2FinalShotDelay;

	// Find next shot
	float nextShotTime = -1.0f;

	if (!isPhase2)
	{
		if (m_shotCount == 0)
		{
			nextShotTime = phase1Shot1Time;
		}
		else if (m_shotCount == 1)
		{
			nextShotTime = phase1Shot2Time;
		}
	}
	else
	{
		if (m_shotCount == 0)
		{
			nextShotTime = phase2Shot1Time;
		}
		else if (m_shotCount == 1)
		{
			nextShotTime = phase2Shot2Time;
		}
		else if (m_shotCount == 2)
		{
			nextShotTime = phase2Shot3Time;
		}
		else if (m_shotCount == 3)
		{
			nextShotTime = phase2FinalShotTime;
		}
	}

	// Tracking + Aim Lock
	ensureValidLockedTarget();
	bool aimLocked = false;

	if (nextShotTime >= 0.0f)
	{
		const float timeUntilShot = nextShotTime - m_stateTimer;
		aimLocked = timeUntilShot <= 0.2f;
	}

	//			Track the player before the shot
	if (!aimLocked)
	{
		const float trackingSpeed = isPhase2 ? config->m_spiritCannonPhase2TrackingSpeed : config->m_spiritCannonTrackingSpeed;
		updateAimDirection(trackingSpeed);
	}

	//			UI
	if (!m_isFuryCast && m_aelorinUI)
	{
		m_aelorinUI->setSpiritCannonAimDirection(m_currentAimDirection);
	}

	// Phase 1
	// Windup -> Shot 1 -> Re-aim -> Shot 2 -> Recovery
	if (!isPhase2)
	{
		if (m_shotCount == 0 && m_stateTimer >= phase1Shot1Time)
		{
			fireBeamShot(
				config->m_spiritCannonBeamWidth,
				config->m_spiritCannonDamage,
				"Spirit Cannon Shot 1"
			);

			++m_shotCount;

			// Telegraph Shot 2
			if (!m_isFuryCast && m_aelorinUI)
			{
				m_aelorinUI->showSpiritCannonUI(
					m_aelorinTransform,
					m_currentAimDirection,
					config->m_spiritCannonBeamLength,
					config->m_spiritCannonBeamWidth,
					config->m_spiritCannonPhase1ShotInterval
				);
			}

			return;
		}

		if (m_shotCount == 1 && m_stateTimer >= phase1Shot2Time)
		{
			fireBeamShot(
				config->m_spiritCannonBeamWidth,
				config->m_spiritCannonDamage,
				"Spirit Cannon Shot 2"
			);

			++m_shotCount;
			return;
		}

		if (m_shotCount >= 2 && m_stateTimer >= phase1Shot2Time + recoveryDuration)
		{
			finishAbility();
		}

		return;
	}

	// Phase 2
	// Shot 1 -> Shot 2 -> Shot 3 -> Final Shot -> Recovery

	if (m_shotCount == 0 && m_stateTimer >= phase2Shot1Time)
	{
		fireBeamShot(
			config->m_spiritCannonBeamWidth,
			config->m_spiritCannonDamage,
			"Spirit Cannon Shot 1"
		);

		++m_shotCount;

		// Telegraph Shot 2
		if (!m_isFuryCast && m_aelorinUI)
		{
			m_aelorinUI->showSpiritCannonUI(
				m_aelorinTransform,
				m_currentAimDirection,
				config->m_spiritCannonBeamLength,
				config->m_spiritCannonBeamWidth,
				config->m_spiritCannonPhase2ShotInterval
			);
		}

		return;
	}

	if (m_shotCount == 1 && m_stateTimer >= phase2Shot2Time)
	{
		fireBeamShot(
			config->m_spiritCannonBeamWidth,
			config->m_spiritCannonDamage,
			"Spirit Cannon Shot 2"
		);

		++m_shotCount;

		// Telegraph Shot 3
		if (!m_isFuryCast && m_aelorinUI)
		{
			m_aelorinUI->showSpiritCannonUI(
				m_aelorinTransform,
				m_currentAimDirection,
				config->m_spiritCannonBeamLength,
				config->m_spiritCannonBeamWidth,
				config->m_spiritCannonPhase2ShotInterval
			);
		}

		return;
	}

	if (m_shotCount == 2 && m_stateTimer >= phase2Shot3Time)
	{
		fireBeamShot(
			config->m_spiritCannonBeamWidth,
			config->m_spiritCannonDamage,
			"Spirit Cannon Shot 3"
		);

		++m_shotCount;

		// Telegraph Final Shot
		if (!m_isFuryCast && m_aelorinUI)
		{
			m_aelorinUI->showSpiritCannonUI(
				m_aelorinTransform,
				m_currentAimDirection,
				config->m_spiritCannonBeamLength,
				config->m_spiritCannonPhase2FinalBeamWidth,
				config->m_spiritCannonPhase2FinalShotDelay
			);
		}

		return;
	}

	if (m_shotCount == 3 && m_stateTimer >= phase2FinalShotTime)
	{
		fireBeamShot(
			config->m_spiritCannonPhase2FinalBeamWidth,
			config->m_spiritCannonPhase2FinalDamage,
			"Spirit Cannon Final Shot"
		);

		++m_shotCount;
		return;
	}

	if (m_shotCount >= 4 && m_stateTimer >= phase2FinalShotTime + recoveryDuration)
	{
		finishAbility();
	}
}

void AelorinSpiritCannonState::OnStateExit()
{
	if (m_aelorinUI)
	{
		m_aelorinUI->cancelSpiritCannon();
	}

	if (m_controller)
	{
		m_controller->clearSpiritCannonDebugLine();
	}

	m_aelorinUI = nullptr;
	m_lockedTarget = nullptr;
	m_currentAimDirection = Vector3::Zero;
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

void AelorinSpiritCannonState::initializeAimDirection()
{
	if (!m_lockedTarget || !m_aelorinTransform)
	{
		return;
	}

	const Vector3 origin = TransformAPI::getGlobalPosition(m_aelorinTransform);
	const Vector3 targetPosition = TransformAPI::getGlobalPosition(m_lockedTarget);
	
	m_currentAimDirection = targetPosition - origin;
	m_currentAimDirection.y = 0.0f;

	if (m_currentAimDirection.LengthSquared() <= 0.00001f)
	{
		m_currentAimDirection = Vector3::Zero;
		return;
	}

	m_currentAimDirection.Normalize();
}

void AelorinSpiritCannonState::updateAimDirection(float trackingSpeed)
{
	if (!m_lockedTarget || !m_aelorinTransform || m_currentAimDirection.LengthSquared() <= 0.00001f)
	{
		return;
	}

	const Vector3 origin = TransformAPI::getGlobalPosition(m_aelorinTransform);
	const Vector3 targetPosition = TransformAPI::getGlobalPosition(m_lockedTarget);

	Vector3 desiredDirection = targetPosition - origin;
	desiredDirection.y = 0.0f;

	if (desiredDirection.LengthSquared() < 0.00001f)
	{
		return;
	}

	desiredDirection.Normalize();

	constexpr float radiansToDegrees = 180.0f / 3.14159265f;
	constexpr float degreesToRadians = 3.14159265f / 180.0f;

	const float currentYaw = std::atan2(m_currentAimDirection.x, m_currentAimDirection.z) * radiansToDegrees;
	const float desiredYaw = std::atan2(desiredDirection.x, desiredDirection.z) * radiansToDegrees;

	float deltaYaw = desiredYaw - currentYaw;

	while (deltaYaw > 180.0f)
	{
		deltaYaw -= 360.0f;
	}

	while (deltaYaw < -180.0f)
	{
		deltaYaw += 360.0f;
	}

	const float maxStep = trackingSpeed * Time::getDeltaTime();

	deltaYaw = std::clamp(deltaYaw, -maxStep, maxStep);
	
	const float newYaw = (currentYaw + deltaYaw) * degreesToRadians;
	
	m_currentAimDirection = Vector3(std::sin(newYaw), 0.0f, std::cos(newYaw));

	m_currentAimDirection.Normalize();
}

void AelorinSpiritCannonState::fireBeamShot(float width, float damage, const char* sourceName)
{
	if (!m_aelorinTransform || !m_attackExecutor || !m_controller)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	if (m_currentAimDirection.LengthSquared() <= 0.00001f)
	{
		return;
	}

	const Vector3 origin = TransformAPI::getGlobalPosition(m_aelorinTransform);

	m_controller->setSpiritCannonDebugLine(origin, m_currentAimDirection, width);

	m_attackExecutor->applyDamageInBeam(origin, m_currentAimDirection, config->m_spiritCannonBeamLength, width, damage, sourceName);
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