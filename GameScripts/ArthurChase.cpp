#include "pch.h"
#include "ArthurChase.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"

ArthurChase::ArthurChase(GameObject* owner)
	: StateMachineScript(owner)
{
}

void ArthurChase::OnStateEnter()
{
	m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
	m_arthurAttackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());

	if (!m_arthurController)
	{
		Debug::error("[ArthurChase] ArthurBossController not found.");
		return;
	}

	if (!m_arthurAttackConfig)
	{
		Debug::error("[ArthurChase] ArthurAttackConfig not found.");
		return;
	}

	m_arthurController->clearPath();
	m_arthurController->resetRepathTimer();
	m_arthurController->updateCurrentTarget();

	if (m_arthurController->hasValidTarget())
	{
		m_arthurController->buildPathToTarget();
	}

	Debug::log("[ArthurChase] ENTER");
}

void ArthurChase::OnStateUpdate()
{

	if (!m_arthurController)
	{
		Debug::error("[ArthurChase] ArthurBossController not found.");
		return;
	}

	if (!m_arthurAttackConfig)
	{
		Debug::error("[ArthurChase] ArthurAttackConfig not found.");
		return;
	}

	AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
	if (!animation)
	{
		Debug::error("[ArthurChase] Animation Component not found.");
		return;
	}

	if (m_arthurController->isDead())
	{
		m_arthurController->clearPath();
		AnimationAPI::sendTrigger(animation, "ToDeath");
		return;
	}

	// Check target

	m_arthurController->updateCurrentTarget();

	if (!m_arthurController->hasValidTarget())
	{
		m_arthurController->clearPath();
		AnimationAPI::sendTrigger(animation, "ToIdle");
		return;
	}

	// Attack checks + State transitions

	// Earth Hammer
	if (m_arthurController->areBothPlayersInEarthHammerRange() && m_arthurController->isEarthHammerReady()) // also need to check if both players are in range
	{
		m_arthurController->consumeEarthHammerCooldown();
		m_arthurController->clearPath();
		AnimationAPI::sendTrigger(animation, "ToEarthHammer");
		return;
	}

	// Side Sweep
	if (m_arthurController->trySelectSideSweepSide() && m_arthurController->isSideSweepReady())
	{
		m_arthurController->clearPath();
		m_arthurController->consumeSideSweepCooldown();
		AnimationAPI::sendTrigger(animation, "ToSideSweep");
		return;
	}

	// Charging Slam
	if (m_arthurController->isTargetInChargingSlamRange() && m_arthurController->isChargingSlamReady())
	{
		m_arthurController->consumeChargingSlamCooldown();
		m_arthurController->clearPath();
		m_arthurController->faceCurrentTarget();
		AnimationAPI::sendTrigger(animation, "ToChargingSlam");
		return;
	}

	// Heavy Swipe
	if (m_arthurController->getDistanceToCurrentTarget() <= m_arthurAttackConfig->m_heavySwipeRange)
	{
		if (m_arthurController->isCurrentTargetInsideHeavySwipeArea(
			m_arthurAttackConfig->m_heavySwipeRange,
			m_arthurAttackConfig->m_heavySwipeHalfAngleDegrees))
		{
			m_arthurController->clearPath();
			m_arthurController->faceCurrentTarget();
			AnimationAPI::sendTrigger(animation, "ToHeavySwipe");
			return;
		}

		m_arthurController->clearPath();
		m_arthurController->faceCurrentTarget();
	}

	// Movement logic
	m_arthurController->addToRepathTimer(Time::getDeltaTime());

	if (m_arthurController->shouldRepath())
	{
		m_arthurController->buildPathToTarget();
		m_arthurController->resetRepathTimer();
	}

	m_arthurController->followPath();
}

void ArthurChase::OnStateExit()
{
	if (!m_arthurController)
	{
		return;
	}

	m_arthurController->clearPath();
	m_arthurController->resetRepathTimer();

	Debug::log("[ArthurChase] EXIT");
}

IMPLEMENT_SCRIPT(ArthurChase)