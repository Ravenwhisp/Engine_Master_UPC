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

	if (!m_arthurController)
	{
		Debug::error("[ArthurRecover] ArthurBossController not found.");
		return;
	}

	m_arthurController->clearPath();
	m_arthurController->resetRepathTimer();
	m_stateTimer = 0.0f;

	Debug::log("[ArthurRecover] ENTER");
}

void ArthurRecover::OnStateUpdate()
{
	m_stateTimer += Time::getDeltaTime();

	AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
	if (!animation)
	{
		Debug::error("[ArthurRecover] Animation Component not found.");
		return;
	}

	if (!m_arthurController)
	{
		Debug::error("[ArthurRecover] ArthurBossController not found.");
		return;
	}

	if (m_arthurController->isDead())
	{
		m_arthurController->clearPath();
		AnimationAPI::sendTrigger(animation, "ToDeath");
		return;
	}

	if (!m_arthurController->hasValidTarget())
	{
		m_arthurController->clearPath();
		AnimationAPI::sendTrigger(animation, "ToIdle");
		return;
	}

	if (m_stateTimer >= m_arthurController->getRecoveryDuration())
	{
		AnimationAPI::sendTrigger(animation, "ToChase");
		return;
	}
}

void ArthurRecover::OnStateExit()
{
	m_stateTimer = 0.0f;
	Debug::log("[ArthurRecover] EXIT");
}

IMPLEMENT_SCRIPT(ArthurRecover)