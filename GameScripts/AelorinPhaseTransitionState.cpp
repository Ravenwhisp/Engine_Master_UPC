#include "pch.h"
#include "AelorinPhaseTransitionState.h"

#include "AelorinBossController.h"

AelorinPhaseTransitionState::AelorinPhaseTransitionState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinPhaseTransitionState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinPhaseTransitionState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	m_phase2Started = false;

	if (!m_controller)
	{
		Debug::error("[AelorinPhaseTransitionState] AelorinBossController not found.");
	}

	if (!m_animation)
	{
		Debug::error("[AelorinPhaseTransitionState] AnimationComponent not found.");
	}

	Debug::log("[AelorinPhaseTransitionState] ENTER");
}

void AelorinPhaseTransitionState::OnStateUpdate()
{
	if (!m_controller || !m_animation)
	{
		return;
	}

	if (m_phase2Started)
	{
		return;
	}

	if (!AnimationAPI::isPlaying(m_animation))
	{
		Debug::log("[AelorinPhaseTransitionState] stopped playing animation");
		m_phase2Started = true;
		m_controller->beginPhase2();
	}
}

void AelorinPhaseTransitionState::OnStateExit()
{
	Debug::log("[AelorinPhaseTransitionState] EXIT");
}

IMPLEMENT_SCRIPT(AelorinPhaseTransitionState)