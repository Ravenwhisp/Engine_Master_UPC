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
	m_animation = AnimationAPI::getAnimationComponent(getOwner());

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

	if (!m_animation)
	{
		Debug::error("[ArthurChase] AnimationComponent not found.");
		return;
	}

	m_arthurController->clearPath();
	m_arthurController->resetRepathTimer();

	Debug::log("[ArthurChase] ENTER");
}

void ArthurChase::OnStateUpdate()
{
	if (!m_arthurController || !m_arthurAttackConfig || !m_animation)
	{
		return;
	}

	if (m_arthurController->trySendDeathTrigger(m_animation))
	{
		return;
	}

	// Check target

	m_arthurController->updateCurrentTarget();

	if (!m_arthurController->hasValidTarget())
	{
		AnimationAPI::sendTrigger(m_animation, "ToIdle");
		return;
	}

	// Attack checks + State transitions

	// Earth Hammer
	if (m_arthurController->areBothPlayersInEarthHammerRange() && m_arthurController->isEarthHammerReady()) // also need to check if both players are in range
	{
		m_arthurController->consumeEarthHammerCooldown();
		AnimationAPI::sendTrigger(m_animation, "ToEarthHammer");
		return;
	}

	// Side Sweep
	if (m_arthurController->trySelectSideSweepSide() && m_arthurController->isSideSweepReady())
	{
		m_arthurController->consumeSideSweepCooldown();

		const int selectedSide = m_arthurController->getSelectedSideSweepSide();

		if (selectedSide == -1)
		{
			AnimationAPI::sendTrigger(m_animation, "ToRightSideSweep");
		}
		else
		{
			AnimationAPI::sendTrigger(m_animation, "ToLeftSideSweep");
		}
		return;
	}

	// Charging Slam
	if (m_arthurController->isTargetInChargingSlamRange() && m_arthurController->isChargingSlamReady())
	{
		m_arthurController->consumeChargingSlamCooldown();
		m_arthurController->faceCurrentTarget();
		AnimationAPI::sendTrigger(m_animation, "ToChargingSlam");
		return;
	}

	// Heavy Swipe
	if (m_arthurController->getDistanceToCurrentTarget() <= m_arthurAttackConfig->m_heavySwipeRange)
	{
		if (m_arthurController->isCurrentTargetInsideHeavySwipeArea(
			m_arthurAttackConfig->m_heavySwipeRange,
			m_arthurAttackConfig->m_heavySwipeHalfAngleDegrees))
		{
			m_arthurController->faceCurrentTarget();
			AnimationAPI::sendTrigger(m_animation, "ToHeavySwipe");
			return;
		}

		m_arthurController->faceCurrentTarget();
	}

	// Movement logic
	m_arthurController->moveTowardsTarget();
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