#include "pch.h"
#include "ArthurDetectionAggro.h"
#include "ArthurBossController.h"
#include "PlayerState.h"
#include "DeathCharacter.h" 

ArthurDetectionAggro::ArthurDetectionAggro(GameObject* owner)
	: EnemyDetectionAggro(owner)
{
}

void ArthurDetectionAggro::Start()
{
	m_arthurBossController = GameObjectAPI::findScript<ArthurBossController>(getOwner());

	if (!m_arthurBossController)
	{
		Debug::error("[ArthurDetectionAggro] ArthurBossController not found.");
		return;
	}
}

void ArthurDetectionAggro::updateAggroState()
{
	if (!m_encounterStarted)
	{
		resetAggro();
		return;
	}

	if (!m_arthurBossController)
	{
		return;
	}

	updateAggroEntries();

	if (isTaunted())
	{
		if (isDowned(m_tauntTargetTransform))
		{
			clearTaunt(m_tauntTargetTransform);
		}
		else
		{
			enterAggro(m_tauntTargetTransform);
			return;
		}
	}

	// Phase 1
	if (m_arthurBossController->getPhase() == ArthurBossPhase::Phase1)
	{
		Transform* lyrielTarget = getLyrielTransform();
		Transform* deathTarget = getDeathTransform();

		if (lyrielTarget && !isDowned(lyrielTarget))
		{
			enterAggro(lyrielTarget);
			return;
		}

		if (deathTarget && !isDowned(deathTarget) && isDowned(lyrielTarget))
		{
			enterAggro(deathTarget);
			return;
		}

		resetAggro();
		return;
	}

	// This is Phase 2 logic
	EnemyDetectionAggro::updateAggroState();
}

void ArthurDetectionAggro::startEncounter()
{
	m_encounterStarted = true;

	resetAggro();
}

void ArthurDetectionAggro::stopEncounter()
{
	m_encounterStarted = false;

	resetAggro();
}

IMPLEMENT_SCRIPT(ArthurDetectionAggro)