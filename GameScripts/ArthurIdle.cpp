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

	if (!m_arthurController)
	{
		Debug::error("[ArthurIdle] ArthurBossController not found.");
		return;
	}

	m_arthurController->clearPath();
	m_arthurController->resetRepathTimer();

	Debug::log("[ArthurIdle] ENTER");
}

void ArthurIdle::OnStateUpdate()
{
	if (!m_arthurController)
	{
		Debug::error("[ArthurIdle] ArthurBossController not found.");
		return;
	}

	AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
	if (!animation)
	{
		Debug::error("[ArthurIdle] Animation Component not found.");
		return;
	}

	if (m_arthurController->isDead())
	{
		m_arthurController->clearPath();
		AnimationAPI::sendTrigger(animation, "ToDeath");
		return;
	}

	m_arthurController->updateCurrentTarget();

	if (!m_arthurController->hasValidTarget())
	{
		return;
	}

	AnimationAPI::sendTrigger(animation, "ToChase");
}

void ArthurIdle::OnStateExit()
{
	Debug::log("[ArthurIdle] EXIT");
}

IMPLEMENT_SCRIPT(ArthurIdle)