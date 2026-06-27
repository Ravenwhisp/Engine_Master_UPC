#include "pch.h"
#include "EnemyIdleState.h"

#include "EnemyBaseController.h"

EnemyIdleState::EnemyIdleState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyIdleState::OnStateEnter()
{
    m_controller = GameObjectAPI::findScript<EnemyBaseController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    if (!m_controller)
    {
        Debug::error("[EnemyIdleState] EnemyController not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[EnemyIdleState] AnimationComponent not found.");
        return;
    }

    m_controller->clearPath();
    m_controller->resetRepathTimer();

    Debug::log("[EnemyIdleState] ENTER");
}

void EnemyIdleState::OnStateUpdate()
{
    if (!m_controller || !m_animation)
    {
        return;
    }

    if (m_controller->trySendDeathTrigger(m_animation))
    {
        return;
    }

    if (m_controller->trySendStunTrigger(m_animation))
    {
        return;
    }

    m_controller->updateCurrentTarget();
    if (!m_controller->hasValidTarget())
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