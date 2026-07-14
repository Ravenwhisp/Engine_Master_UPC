#include "pch.h"
#include "SummonerSummonState.h"

#include "SummonerEnemyController.h"
#include "SummonerAttackConfig.h"

SummonerSummonState::SummonerSummonState(GameObject* owner)
	: StateMachineScript(owner)
{
}

void SummonerSummonState::OnStateEnter()
{
    m_controller = GameObjectAPI::findScript<SummonerEnemyController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

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

	Debug::log("[SummonerSummonState] ENTER");
}

void SummonerSummonState::OnStateUpdate()
{
    if (!m_controller || !m_animation)
    {
        return;
    }

    if (m_controller->trySendDeathTrigger(m_animation))
    {
        return;
    }

    const SummonerAttackConfig* cfg = m_attackConfig.get();
    if (!cfg)
    {
        return;
    }

    m_stateTimer += Time::getDeltaTime();

    if (!m_hasSummoned && m_stateTimer >= cfg->m_summonCastTime)
    {
        m_controller->summonSpidersAroundSelf();
        m_controller->consumeSummonCooldown();
        m_hasSummoned = true;
    }

    if (m_stateTimer >= cfg->m_summonTotalDuration)
	{
		AnimationAPI::sendTrigger(m_animation, "ToRecover");
	}
}

void SummonerSummonState::OnStateExit()
{
	m_stateTimer = 0.0f;
	m_hasSummoned = false;

	Debug::log("[SummonerSummonState] EXIT");
}

IMPLEMENT_SCRIPT_FIELDS(SummonerSummonState,
    SERIALIZED_ASSET_REF(m_attackConfig, "Attack Config", AssetType::DATA_CONTAINER)
)