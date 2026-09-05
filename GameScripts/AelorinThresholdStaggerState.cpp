#include "pch.h"
#include "AelorinThresholdStaggerState.h"

#include "AelorinBossController.h"

AelorinThresholdStaggerState::AelorinThresholdStaggerState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void AelorinThresholdStaggerState::OnStateEnter()
{
	Transform* parentTransform = TransformAPI::getParent(getOwner()->GetTransform());
	if (!parentTransform)
	{
		Debug::error("[AelorinThresholdStaggerState] Aelorin transform not found.");
		return;
	}

	GameObject* parentGameObject = ComponentAPI::getOwner(parentTransform);

	m_controller = GameObjectAPI::findScript<AelorinBossController>(parentGameObject);
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_staggerCompleted = false;
	m_staggerTimer = 0.0f;

	if (!m_controller)
	{
		Debug::error("[AelorinThresholdStaggerState] AelorinBossController not found.");
	}

	if (!m_animation)
	{
		Debug::error("[AelorinThresholdStaggerState] AnimationComponent not found.");
	}

	Debug::log("[AelorinThresholdStaggerState] ENTER");
}

void AelorinThresholdStaggerState::OnStateUpdate()
{
	if (!m_controller || !m_animation || m_staggerCompleted)
	{
		return;
	}

	m_staggerTimer += Time::getDeltaTime();

	if (m_staggerTimer < m_controller->getThresholdStaggerDuration())
	{
		return;
	}

	m_staggerCompleted = true;

	m_controller->completeThresholdStagger();

	const bool sent = AnimationAPI::sendTrigger(m_animation, "ToIdle");

	if (!sent)
	{
		Debug::warn("[AelorinThresholdStaggerState] Failed to send ToIdle trigger.");
	}
}

void AelorinThresholdStaggerState::OnStateExit()
{
	Debug::log("[AelorinThresholdStaggerState] EXIT");
}

IMPLEMENT_SCRIPT(AelorinThresholdStaggerState)