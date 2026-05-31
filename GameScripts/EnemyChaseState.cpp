#include "pch.h"
#include "EnemyChaseState.h"

#include "RangedEnemyController.h"

EnemyChaseState::EnemyChaseState(GameObject* owner)
    : StateMachineScript(owner)
{
}

void EnemyChaseState::OnStateEnter()
{
    m_archerController = GameObjectAPI::findScript<RangedEnemyController>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());

    if (!m_archerController)
    {
        Debug::error("[EnemyChaseState] RangedEnemyController not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[EnemyChaseState] AnimationComponent not found.");
        return;
    }

    Debug::log("[EnemyChaseState] ENTER");
}

void EnemyChaseState::OnStateUpdate()
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
        AnimationAPI::sendTrigger(m_animation, "ToIdle");

        Debug::log("[EnemyChaseState] Idle trigger sent");

        return;
    }

    if (m_archerController->playerInSomersaultRange() && m_archerController->isSomersaultReady())
    {
        AnimationAPI::sendTrigger(m_animation, "ToSomersault");
        Debug::log("[EnemyChaseState] Somersault trigger sent");
        return;
    }

    if (m_archerController->isTargetInArrowBarrageRange()  && m_archerController->isArrowBarrageReady())
    {
        AnimationAPI::sendTrigger(m_animation, "ToArrowBarrage");
        Debug::log("[EnemyChaseState] Arrow Barrage trigger sent");
        return;
    }

    if (m_archerController->isTargetInAttackRange())
    {
        AnimationAPI::sendTrigger(m_animation, "ToAttack");

        Debug::log("[EnemyChaseState] Attack trigger sent");

        return;
    }

    m_archerController->moveTowardsTarget();
}

void EnemyChaseState::OnStateExit()
{
    Debug::log("[EnemyChaseState] EXIT");

    if (!m_archerController)
    {
        return;
    }

    m_archerController->clearPath();
}

IMPLEMENT_SCRIPT(EnemyChaseState)