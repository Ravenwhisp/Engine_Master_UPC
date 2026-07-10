#include "pch.h"
#include "ArcherChaseState.h"

#include "RangedEnemyController.h"

ArcherChaseState::ArcherChaseState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArcherChaseState::OnStateEnter()
{
    m_archerController = GameObjectAPI::findScript<RangedEnemyController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    if (!m_archerController)
    {
        Debug::error("[ArcherChaseState] RangedEnemyController not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ArcherChaseState] AnimationComponent not found.");
        return;
    }

    m_archerController->clearPath();
    m_archerController->resetRepathTimer();

    Debug::log("[ArcherChaseState] ENTER");
}

void ArcherChaseState::OnStateUpdate()
{
    if (!m_archerController || !m_animation)
    {
        return;
    }

    if (m_archerController->trySendDeathTrigger(m_animation))
    {
        return;
    }

    if (m_archerController->trySendStunTrigger(m_animation))
    {
        return;
    }

    if (!m_archerController->hasValidTarget())
    {
        AnimationAPI::sendTrigger(m_animation, "ToIdle");

        Debug::log("[ArcherChaseState] Idle trigger sent");

        return;
    }

    if (m_archerController->playerInSomersaultRange() && m_archerController->isSomersaultReady())
    {
        AnimationAPI::sendTrigger(m_animation, "ToSomersault");
        Debug::log("[ArcherChaseState] Somersault trigger sent");
        return;
    }

    if (m_archerController->isTargetInArrowBarrageRange()  && m_archerController->isArrowBarrageReady())
    {
        AnimationAPI::sendTrigger(m_animation, "ToArrowBarrage");
        Debug::log("[ArcherChaseState] Arrow Barrage trigger sent");
        return;
    }

    if (m_archerController->isTargetInAttackRange())
    {
        AnimationAPI::sendTrigger(m_animation, "ToAttack");

        Debug::log("[ArcherChaseState] Attack trigger sent");

        return;
    }

    m_archerController->moveTowardsTarget();
}

void ArcherChaseState::OnStateExit()
{
    Debug::log("[ArcherChaseState] EXIT");

    if (!m_archerController)
    {
        return;
    }

    m_archerController->clearPath();
    m_archerController->resetRepathTimer();
}

IMPLEMENT_SCRIPT(ArcherChaseState)