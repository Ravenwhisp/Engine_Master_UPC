#include "pch.h"
#include "ArthurIdle.h"

#include "ArthurBossController.h"

ArthurIdle::ArthurIdle(GameObject* owner)
	: StateMachineScript(owner)
{
}

void ArthurIdle::OnStateEnter()
{
	m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

	if (!m_arthurController)
	{
		Debug::error("[ArthurIdle] ArthurBossController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[ArthurIdle] AnimationComponent not found.");
		return;
	}

	m_arthurController->clearPath();
	m_arthurController->resetRepathTimer();

	Debug::log("[ArthurIdle] ENTER");
}

void ArthurIdle::OnStateUpdate()
{
	if (!m_arthurController || !m_animation)
	{
		return;
	}

	if (m_arthurController->trySendDeathTrigger(m_animation))
	{
		return;
	}

	m_arthurController->updateCurrentTarget();

	if (!m_arthurController->hasValidTarget())
	{
		return;
	}

	AnimationAPI::sendTrigger(m_animation, "ToChase");
}

void ArthurIdle::OnStateExit()
{
	Debug::log("[ArthurIdle] EXIT");
}

IMPLEMENT_SCRIPT(ArthurIdle)