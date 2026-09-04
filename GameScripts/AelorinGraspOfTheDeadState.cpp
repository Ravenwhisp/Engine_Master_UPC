#include "pch.h"
#include "AelorinGraspOfTheDeadState.h"

#include "AelorinAttackConfig.h"
#include "AelorinAttackExecutor.h"
#include "AelorinUI.h"

#include "PlayerMovement.h"

#include <algorithm>

AelorinGraspOfTheDeadState::AelorinGraspOfTheDeadState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinGraspOfTheDeadState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinGraspOfTheDeadState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	// get scripts
	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_aelorinUI = GameObjectAPI::findScript<AelorinUI>(parentGameObject);

	// reset members
	m_lyrielMovement = nullptr;
	m_deathMovement = nullptr;
	m_activeAbility = AelorinAbility::None;
	m_stateTimer = 0.0f;
	m_completed = false;
	m_isFuryCast = false;

	if (!m_controller)
	{
		Debug::error("[AelorinGraspOfTheDeadState] AelorinBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[AelorinGraspOfTheDeadState] AnimationComponent not found.");
		return;
	}

	if (!m_aelorinUI)
	{
		Debug::error("[AelorinGraspOfTheDeadState] AelorinUI not found.");
		return;
	}

	m_attackExecutor = m_controller->getAttackExecutor();

	if (!m_attackExecutor)
	{
		Debug::error("[AelorinGraspOfTheDeadState] AelorinAttackExecutor not found.");
		return;
	}

	// consume ability
	m_activeAbility = m_controller->consumeRequestedAbility();
	if (m_activeAbility != AelorinAbility::GraspOfTheDead)
	{
		Debug::warn("[AelorinGraspOfTheDeadState] Unexpected requested ability!");
		return;
	}

	if (!m_controller->getGraspCenter())
	{
		Debug::warn("[AelorinGraspOfTheDeadState] Grasp Center not assigned.");
		return;
	}

	// cache PlayerMovement scripts
	Transform* lyrielTransform = m_controller->getLyrielTransform();
	if (lyrielTransform)
	{
		GameObject* lyrielObject = ComponentAPI::getOwner(lyrielTransform);
		if (lyrielObject)
		{
			m_lyrielMovement = GameObjectAPI::findScript<PlayerMovement>(lyrielObject);
		}
	}

	Transform* deathTransform = m_controller->getDeathTransform();
	if (deathTransform)
	{
		GameObject* deathObject = ComponentAPI::getOwner(deathTransform);
		if (deathObject)
		{
			m_deathMovement = GameObjectAPI::findScript<PlayerMovement>(deathObject);
		}
	}

	if (!m_lyrielMovement)
	{
		Debug::warn("[AelorinGraspOfTheDeadState] Lyriel PlayerMovement not found.");
	}

	if (!m_deathMovement)
	{
		Debug::warn("[AelorinGraspOfTheDeadState] Death PlayerMovement not found.");
	}

	m_isFuryCast = m_controller->isFuryActive();
	if (m_isFuryCast)
	{
		m_controller->recordFuryCast();
	}

	// UI
	if (!m_isFuryCast && m_aelorinUI)
	{
		const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
		Transform* graspCenter = m_controller->getGraspCenter();

		if (config && graspCenter)
		{
			const Vector3 center = TransformAPI::getGlobalPosition(graspCenter);
			m_aelorinUI->showGraspOfTheDeadUI(center, config->m_graspVisualRadius, config->m_graspPullDuration);
		}
	}

	Debug::log("[AelorinGraspOfTheDeadState] ENTER");
}

void AelorinGraspOfTheDeadState::OnStateUpdate()
{
	if (!m_controller || !m_attackExecutor || !m_animation || m_completed)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	m_stateTimer += Time::getDeltaTime();

	// Pull both players continuously towards the grasp center for a duration
	if (m_stateTimer < config->m_graspPullDuration)
	{
		pullPlayer(m_controller->getLyrielTransform(), m_lyrielMovement);
		pullPlayer(m_controller->getDeathTransform(), m_deathMovement);

		return;
	}

	// transition into Nova
	chainIntoNova();
}

void AelorinGraspOfTheDeadState::OnStateExit()
{
	m_aelorinUI = nullptr;
	m_lyrielMovement = nullptr;
	m_deathMovement = nullptr;
	m_stateTimer = 0.0f;
	m_completed = false;
	m_isFuryCast = false;

	Debug::log("[AelorinGraspOfTheDeadState] EXIT");
}

void AelorinGraspOfTheDeadState::pullPlayer(Transform* playerTransform, PlayerMovement* playerMovement)
{
	if (!playerTransform || !playerMovement || !m_controller || !m_attackExecutor)
	{
		return;
	}

	if (!m_attackExecutor->isValidPlayerTarget(playerTransform))
	{
		return;
	}

	Transform* graspCenter = m_controller->getGraspCenter();
	if (!graspCenter)
	{
		return;
	}

	GameObject* playerObject = ComponentAPI::getOwner(playerTransform);
	if (!playerObject)
	{
		return;
	}

	const AelorinAttackConfig* config = m_controller->getAelorinAttackConfig();
	if (!config)
	{
		return;
	}

	const Vector3 playerPosition = TransformAPI::getGlobalPosition(playerTransform);
	const Vector3 centerPosition = TransformAPI::getGlobalPosition(graspCenter);

	Vector3 pullDirection = centerPosition - playerPosition;
	pullDirection.y = 0.0f;

	const float distance = pullDirection.Length();
	if (distance <= 0.0001f)
	{
		return;
	}

	pullDirection.Normalize();

	const float pullDistance = config->m_graspPullStrength * Time::getDeltaTime();

	// pull must not overshoot the center
	const float clampedPullDistance = (std::min)(pullDistance, distance);

	playerMovement->applyExternalMovement(playerObject, pullDirection * clampedPullDistance);
}

void AelorinGraspOfTheDeadState::chainIntoNova()
{
	if (m_completed || !m_controller || !m_animation)
	{
		return;
	}

	Transform* graspCenter = m_controller->getGraspCenter();
	if (!graspCenter)
	{
		Debug::error("[AelorinGraspOfTheDeadState] Grasp Center not found.");
		return;
	}

	const Vector3 center = TransformAPI::getGlobalPosition(graspCenter);
	m_controller->prepareForcedNovaAt(center);

	m_completed = true;

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToNova");
	if (!sent)
	{
		Debug::warn("[AelorinGraspOfTheDeadState] Failed to send ToNova trigger.");
	}
}

IMPLEMENT_SCRIPT(AelorinGraspOfTheDeadState)