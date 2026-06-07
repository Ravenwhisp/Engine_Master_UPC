#include "pch.h"
#include "EnemyIdleState.h"

#include "RangedEnemyController.h"

EnemyIdleState::EnemyIdleState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyIdleState::OnStateEnter()
{
    m_archerController = GameObjectAPI::findScript<RangedEnemyController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    if (!m_archerController)
    {
        Debug::error("[EnemyIdle] RangedEnemyController not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[EnemyIdle] AnimationComponent not found.");
        return;
    }

    Debug::log("[EnemyIdleState] ENTER");
}

void EnemyIdleState::OnStateUpdate()
{
    if (!m_archerController || !m_animation)
    {
        return;
    }

    if (m_archerController->trySendDeathTrigger(m_animation))
    {
        return;
    }

    if (!m_archerController->hasTarget())
    {
        return;
    }

    AnimationAPI::sendTrigger(m_animation, "ToChase");

    Debug::log("[EnemyIdleState] Chase trigger sent");
}

void EnemyIdleState::OnStateExit()
{
    Debug::log("[EnemyIdleState] EXIT");
}

IMPLEMENT_SCRIPT(EnemyIdleState)