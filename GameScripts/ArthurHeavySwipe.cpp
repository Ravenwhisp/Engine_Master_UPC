#include "pch.h"
#include "ArthurHeavySwipe.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "EnemyAttackExecutor.h"
#include "ArthurUI.h"
#include "ArthurSound.h"

ArthurHeavySwipe::ArthurHeavySwipe(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurHeavySwipe::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());
    m_arthurUI = GameObjectAPI::findScript<ArthurUI>(getOwner());
    m_arthurSound = GameObjectAPI::findScript<ArthurSound>(getOwner());

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
        Debug::error("[ArthurHeavySwipe] EnemyAttackExecutor not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ArthurHeavySwipe] AnimationComponent not found.");
        return;
    }

    if (!m_arthurUI)
    {
        Debug::error("[ArthurHeavySwipe] ArthurUI not found.");
        return;
    }

    m_previousAnimationSpeed = AnimationAPI::getSpeedMultiplier(m_animation);

    float animationSpeed = m_phase1AnimationSpeed;

    if (m_arthurController->isPhase2())
    {
        animationSpeed = m_phase2AnimationSpeed;
    }

    AnimationAPI::setSpeedMultiplier(m_animation, animationSpeed);

    m_arthurController->clearPath();
    m_arthurController->resetRepathTimer();
    m_arthurController->updateCurrentTarget();
    m_arthurController->faceCurrentTarget();

    m_arthurUI->setupHeavySwipeUI();

    Debug::log("[ArthurHeavySwipe] ENTER");
}

void ArthurHeavySwipe::OnStateUpdate()
{
    if (!m_arthurController || !m_attackConfig || !m_attackExecutor || !m_animation)
    {
        return;
    }

    if (m_arthurController->trySendDeathTrigger(m_animation))
    {
        return;
    }

    m_stateTimer += Time::getDeltaTime();

    const bool isPhase2 = m_arthurController->isPhase2();

    float hit1Time = m_attackConfig->m_heavySwipeHit1Time;
    float hit2Time = m_attackConfig->m_heavySwipeHit2Time;
    float hit3Time = m_attackConfig->m_heavySwipeHit3Time;
    float hit4Time = m_attackConfig->m_heavySwipePhase2Hit4Time;

    if (isPhase2)
    {
        hit1Time = m_attackConfig->m_heavySwipePhase2Hit1Time;
        hit2Time = m_attackConfig->m_heavySwipePhase2Hit2Time;
        hit3Time = m_attackConfig->m_heavySwipePhase2Hit3Time;
    }

    if (m_arthurUI)
    {
        m_arthurUI->updateHeavySwipeUI(m_stateTimer, isPhase2, hit1Time, hit2Time, hit3Time, hit4Time, m_attackConfig->m_heavySwipeTotalDuration, m_attackConfig->m_heavySwipeRange);
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

    if (isPhase2 && !m_hit4Applied && m_stateTimer >= hit4Time)
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

    if (m_arthurUI)
    {
        m_arthurUI->hideHeavySwipeUI();
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

    const int hits = m_attackExecutor->applyDamageInCone(center, forward, m_attackConfig->m_heavySwipeRange, m_attackConfig->m_heavySwipeHalfAngleDegrees, m_attackConfig->m_heavySwipeDamage, "HeavySwipe");

    if (m_arthurSound)
    {
        m_arthurSound->playClawSwipe();          // swoosh on every strike of the combo
        if (hits > 0)
        {
            m_arthurSound->playClawImpact();     // impact only when the strike connects
        }
    }

    Debug::log("[ArthurHeavySwipe] Hit %d applied.", hitIndex);
}

void ArthurHeavySwipe::goToRecover()
{
    if (!m_attackConfig || !m_animation)
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

    AnimationAPI::sendTrigger(m_animation, "ToRecover");
}

IMPLEMENT_SCRIPT(ArthurHeavySwipe)