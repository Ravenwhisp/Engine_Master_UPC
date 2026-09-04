#include "pch.h"
#include "AelorinDeathState.h"

#include "AelorinBossController.h"

AelorinDeathState::AelorinDeathState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinDeathState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinDeathState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	m_animationFinished = false;

	if (!m_controller)
	{
		Debug::error("[AelorinDeathState] AelorinBossController not found.");
	}

	if (!m_animation)
	{
		Debug::error("[AelorinDeathState] AnimationComponent not found.");
	}

	if (m_controller)
	{
		m_controller->clearPath();
		m_controller->resetRepathTimer();

		// Nothing should move Aelorin anymore.
		m_controller->setForcedMovementActive(false);
		m_controller->setForcedMovementBlocked(true);
	}

	Debug::log("[AelorinDeathState] ENTER");
}

void AelorinDeathState::OnStateUpdate()
{
	if (!m_animation || m_animationFinished)
	{
		return;
	}

	if (!AnimationAPI::isPlaying(m_animation))
	{
		m_animationFinished = true;
		Debug::log("[AelorinDeathState] Death animation finished.");
	}
}

void AelorinDeathState::OnStateExit()
{
	// this should normally never happen.
	Debug::log("[AelorinDeathState] EXIT");
}

IMPLEMENT_SCRIPT(AelorinDeathState)