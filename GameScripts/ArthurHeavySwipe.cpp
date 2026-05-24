#include "pch.h"
#include "ArthurHeavySwipe.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "ArthurAttackExecutor.h"

ArthurHeavySwipe::ArthurHeavySwipe(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurHeavySwipe::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<ArthurAttackExecutor>(getOwner());

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
    if (animation)
    {
        m_previousAnimationSpeed = AnimationAPI::getSpeedMultiplier(animation);

        float animationSpeed = m_phase1AnimationSpeed;

        if (m_arthurController->isPhase2())
        {
            animationSpeed = m_phase2AnimationSpeed;
        }

        AnimationAPI::setSpeedMultiplier(animation, animationSpeed);
    }

    m_stateTimer = 0.0f;

    m_hit1Applied = false;
    m_hit2Applied = false;
    m_hit3Applied = false;
    m_hit4Applied = false;

    if (!m_arthurController)
    {
        Debug::error("[ArthurHeavySwipe] ArthurBossController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[ArthurHeavySwipe] ArthurAttackConfig not found.");
        return;
    }

    if (!m_attackExecutor)
    {
        Debug::error("[ArthurHeavySwipe] ArthurAttackExecutor not found.");
        return;
    }

    m_arthurController->clearPath();
    m_arthurController->updateCurrentTarget();
    m_arthurController->faceCurrentTarget();

    Debug::log("[ArthurHeavySwipe] ENTER");
}

void ArthurHeavySwipe::OnStateUpdate()
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

    const bool isPhase2 = m_arthurController->isPhase2();

    float hit1Time = m_attackConfig->m_heavySwipeHit1Time;
    float hit2Time = m_attackConfig->m_heavySwipeHit2Time;
    float hit3Time = m_attackConfig->m_heavySwipeHit3Time;

    if (isPhase2)
    {
        hit1Time = m_attackConfig->m_heavySwipePhase2Hit1Time;
        hit2Time = m_attackConfig->m_heavySwipePhase2Hit2Time;
        hit3Time = m_attackConfig->m_heavySwipePhase2Hit3Time;
    }

    if (!m_hit1Applied && m_stateTimer >= hit1Time)
    {
        tryApplyHit(1);
        m_hit1Applied = true;
    }

    if (!m_hit2Applied && m_stateTimer >= hit2Time)
    {
        tryApplyHit(2);
        m_hit2Applied = true;
    }

    if (!m_hit3Applied && m_stateTimer >= hit3Time)
    {
        tryApplyHit(3);
        m_hit3Applied = true;
    }

    if (isPhase2 && !m_hit4Applied && m_stateTimer >= m_attackConfig->m_heavySwipePhase2Hit4Time)
    {
        tryApplyHit(4);
        m_hit4Applied = true;
    }

    if (m_stateTimer >= m_attackConfig->m_heavySwipeTotalDuration)
    {
        goToRecover();
        return;
    }
}

void ArthurHeavySwipe::OnStateExit()
{
    AnimationComponent* animation = AnimationAPI::getAnimationComponent(getOwner());
    if (animation)
    {
        AnimationAPI::setSpeedMultiplier(animation, m_previousAnimationSpeed);
    }

    Debug::log("[ArthurHeavySwipe] EXIT");
}

void ArthurHeavySwipe::tryApplyHit(int hitIndex)
{
    if (!m_attackConfig || !m_attackExecutor)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 center = TransformAPI::getGlobalPosition(ownerTransform);
    Vector3 forward = TransformAPI::getForward(ownerTransform);

    m_attackExecutor->applyDamageInCone(center, forward, m_attackConfig->m_heavySwipeRange, m_attackConfig->m_heavySwipeHalfAngleDegrees, m_attackConfig->m_heavySwipeDamage, "HeavySwipe");

    Debug::log("[ArthurHeavySwipe] Hit %d applied.", hitIndex);
}

void ArthurHeavySwipe::goToRecover()
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
        float recoveryDuration = m_attackConfig->m_heavySwipeRecoveryDuration;

        if (m_arthurController->isPhase2())
        {
            recoveryDuration = m_attackConfig->m_heavySwipePhase2RecoveryDuration;
        }

        m_arthurController->setRecoveryDuration(recoveryDuration);
    }

    Debug::log("[ArthurHeavySwipe] Going to Recover.");

    AnimationAPI::sendTrigger(animation, "ToRecover");
}

IMPLEMENT_SCRIPT(ArthurHeavySwipe)