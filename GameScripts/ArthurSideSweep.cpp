#include "pch.h"
#include "ArthurSideSweep.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "ArthurAttackExecutor.h"

ArthurSideSweep::ArthurSideSweep(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurSideSweep::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<ArthurAttackExecutor>(getOwner());

    m_stateTimer = 0.0f;
    m_hasAppliedHit = false;

    if (!m_arthurController)
    {
        Debug::error("[ArthurSideSweep] ArthurBossController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[ArthurSideSweep] ArthurAttackConfig not found.");
        return;
    }

    if (!m_attackExecutor)
    {
        Debug::error("[ArthurSideSweep] ArthurAttackExecutor not found.");
        return;
    }

    m_arthurController->clearPath();
    m_arthurController->updateCurrentTarget();

    m_sweepSide = m_arthurController->getSelectedSideSweepSide();

    Debug::log("[ArthurSideSweep] ENTER");
}

void ArthurSideSweep::OnStateUpdate()
{
    if (!m_arthurController || !m_attackConfig || !m_attackExecutor)
    {
        return;
    }

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
    if (!animation)
    {
        return;
    }

    if (m_arthurController->isDead())
    {
        m_arthurController->clearPath();
        AnimationAPI::sendTrigger(animation, "ToDeath");
        return;
    }

    m_stateTimer += Time::getDeltaTime();

    float hitTime = m_attackConfig->m_sideSweepHitTime;
    float totalDuration = m_attackConfig->m_sideSweepTotalDuration;

    if (m_arthurController->isPhase2())
    {
        hitTime = m_attackConfig->m_sideSweepPhase2HitTime;
        totalDuration = m_attackConfig->m_sideSweepPhase2TotalDuration;
    }

    if (!m_hasAppliedHit && m_stateTimer >= hitTime)
    {
        applyHit();
        m_hasAppliedHit = true;
    }

    if (m_stateTimer >= totalDuration)
    {
        goToRecover();
        return;
    }
}

void ArthurSideSweep::OnStateExit()
{
    Debug::log("[ArthurSideSweep] EXIT");
}

void ArthurSideSweep::applyHit()
{
    if (!m_arthurController || !m_attackConfig || !m_attackExecutor)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 center = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 sweepDirection = m_arthurController->getSideSweepDirection(m_sweepSide);

    m_attackExecutor->applyDamageInCone(center, sweepDirection, m_attackConfig->m_sideSweepRange, m_attackConfig->m_sideSweepHalfAngleDegrees, m_attackConfig->m_sideSweepDamage, "SideSweep");

    Debug::log("[ArthurSideSweep] Hit applied. Side: %d", m_sweepSide);
}

void ArthurSideSweep::goToRecover()
{
    if (!m_attackConfig)
    {
        return;
    }

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
    if (!animation)
    {
        return;
    }

    if (m_arthurController)
    {
        float recoveryDuration = m_attackConfig->m_sideSweepRecoveryDuration;

        if (m_arthurController->isPhase2())
        {
            recoveryDuration = m_attackConfig->m_sideSweepPhase2RecoveryDuration;
        }

        m_arthurController->setRecoveryDuration(recoveryDuration);
    }

    Debug::log("[ArthurSideSweep] Going to Recover.");

    AnimationAPI::sendTrigger(animation, "ToRecover");
}

IMPLEMENT_SCRIPT(ArthurSideSweep)