#include "pch.h"
#include "ArthurRecover.h"

#include "ArthurBossController.h"

ArthurRecover::ArthurRecover(GameObject* owner)
	: StateMachineScript(owner)
{
}

void ArthurRecover::OnStateEnter()
{
	m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_arthurController)
	{
		Debug::error("[ArthurRecover] ArthurBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[ArthurRecover] AnimationComponent not found.");
		return;
	}

	m_arthurController->clearPath();
	m_arthurController->resetRepathTimer();
	m_stateTimer = 0.0f;

	Debug::log("[ArthurRecover] ENTER");
}

void ArthurRecover::OnStateUpdate()
{
	if (!m_arthurController || !m_animation)
	{
		return;
	}

	m_stateTimer += Time::getDeltaTime();

	if (m_arthurController->trySendDeathTrigger(m_animation))
	{
		return;
	}

	if (!m_arthurController->hasValidTarget())
	{
		m_arthurController->clearPath();
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	if (m_stateTimer >= m_arthurController->getRecoveryDuration())
	{
		AnimationAPI::sendTrigger(m_animation, "ToChase");
		return;
	}
}

void ArthurRecover::OnStateExit()
{
	m_stateTimer = 0.0f;
	Debug::log("[ArthurRecover] EXIT");
}

IMPLEMENT_SCRIPT(ArthurRecover)