#include "pch.h"
#include "SummonerSummonState.h"

#include "SummonerEnemyController.h"
#include "SummonerAttackConfig.h"
#include "SummonerParticles.h"

SummonerSummonState::SummonerSummonState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SummonerSummonState::OnStateEnter()
{
	m_controller = GameObjectAPI::findScript<SummonerEnemyController>(getOwner());
	m_animation = AnimationAPI::getAnimationComponent(getOwner());
	m_particles = GameObjectAPI::findScript<SummonerParticles>(getOwner());

	if (!m_controller)
	{
		Debug::error("[SummonerSummonState] EnemyController not found.");
		return;
	}

	if (!m_animation)
	{
		Debug::error("[SummonerSummonState] AnimationComponent not found.");
		return;
	}

	m_stateTimer = 0.0f;
	m_hasSummoned = false;
	m_hasScheduledSummonVfx = false;
	m_plannedSummonPositions.clear();

	if (m_controller)
	{
		m_controller->computeSummonSpawnPositions(m_plannedSummonPositions);
	}

	Debug::log("[SummonerSummonState] ENTER");
}

void SummonerSummonState::OnStateUpdate()
{
	if (!m_controller || !m_controller->m_attackConfig || !m_animation)
	{
		return;
	}

	if (m_controller->trySendDeathTrigger(m_animation))
	{
		return;
	}

	m_stateTimer += Time::getDeltaTime();

	const float summonCastTime = m_controller->m_attackConfig.get()->m_summonCastTime;
	const float summonVfxLeadTime = 1.0f;
	const float spiderSpawnTime = summonCastTime > summonVfxLeadTime ? summonCastTime : summonVfxLeadTime;
	const float summonVfxDelay = spiderSpawnTime - summonVfxLeadTime;

	if (!m_hasScheduledSummonVfx && m_particles && !m_plannedSummonPositions.empty())
	{
		if (m_stateTimer >= summonVfxDelay)
		{
			for (const Vector3& spawnPosition : m_plannedSummonPositions)
			{
				m_particles->playSummonParticle(spawnPosition);
			}

			m_hasScheduledSummonVfx = true;
		}
	}

	if (!m_hasSummoned && m_stateTimer >= spiderSpawnTime)
	{
		m_controller->summonSpidersAtPositions(m_plannedSummonPositions);
		m_controller->consumeSummonCooldown();
		m_hasSummoned = true;
	}

	if (m_stateTimer >= m_controller->m_attackConfig.get()->m_summonTotalDuration)
	{
		AnimationAPI::sendTrigger(m_animation, "ToRecover");
	}
}

void SummonerSummonState::OnStateExit()
{
	m_stateTimer = 0.0f;
	m_hasSummoned = false;
	m_hasScheduledSummonVfx = false;
	m_plannedSummonPositions.clear();

	Debug::log("[SummonerSummonState] EXIT");
}

IMPLEMENT_SCRIPT(SummonerSummonState)
