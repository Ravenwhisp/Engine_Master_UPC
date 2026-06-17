#include "pch.h"
#include "ArthurEarthHammer.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "EnemyAttackExecutor.h"
#include "ArthurUI.h"

ArthurEarthHammer::ArthurEarthHammer(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurEarthHammer::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackConfig = GameObjectAPI::findScript<ArthurAttackConfig>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());
    m_arthurUI = GameObjectAPI::findScript<ArthurUI>(getOwner());

    m_stateTimer = 0.0f;
    m_hasAppliedImpact = false;

    if (!m_arthurController)
    {
        Debug::error("[ArthurEarthHammer] ArthurBossController not found.");
        return;
    }

    if (!m_attackConfig)
    {
        Debug::error("[ArthurEarthHammer] ArthurAttackConfig not found.");
        return;
    }

    if (!m_attackExecutor)
    {
        Debug::error("[ArthurEarthHammer] EnemyAttackExecutor not found.");
        return;
    }

    if (!m_animation)
    {
        Debug::error("[ArthurEarthHammer] AnimationComponent not found.");
        return;
    }

    if (!m_arthurUI)
    {
        Debug::error("[ArthurEarthHammer] ArthurUI not found.");
        return;
    }

    m_arthurController->clearPath();
    m_arthurController->resetRepathTimer();

    m_arthurController->updateCurrentTarget();
    m_arthurController->faceCurrentTarget();

    m_arthurUI->setupEarthHammerUI();

    Debug::log("[ArthurEarthHammer] ENTER");
}

void ArthurEarthHammer::OnStateUpdate()
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

    if (m_arthurUI)
    {
        m_arthurUI->updateEarthHammerUI(m_stateTimer, m_hasAppliedImpact, m_attackConfig->m_earthHammerHitTime, m_attackConfig->m_earthHammerRecoveryDuration);
    }

    if (!m_hasAppliedImpact && m_stateTimer >= m_attackConfig->m_earthHammerHitTime)
    {
        applyImpact();
        m_hasAppliedImpact = true;
    }

    if (m_stateTimer >= m_attackConfig->m_earthHammerTotalDuration)
    {
        goToRecover();
        return;
    }
}

void ArthurEarthHammer::OnStateExit()
{
    if (m_arthurUI)
    {
        m_arthurUI->hideEarthHammerUI();
    }

    Debug::log("[ArthurEarthHammer] EXIT");
}

void ArthurEarthHammer::applyImpact()
{
    if (!m_arthurController || !m_attackExecutor || !m_attackConfig)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (!ownerTransform)
    {
        return;
    }

    Vector3 center = TransformAPI::getGlobalPosition(ownerTransform);

    const bool isPhase2 = m_arthurController->isPhase2();

    float damage = m_attackConfig->m_earthHammerDamage;
    float stunDuration = m_attackConfig->m_earthHammerStunDuration;

    if (isPhase2)
    {
        damage = m_attackConfig->m_earthHammerPhase2Damage;
        stunDuration = m_attackConfig->m_earthHammerPhase2StunDuration;
    }

    m_attackExecutor->applyDamageAndStunInRadius(center, m_attackConfig->m_earthHammerRadius, damage, stunDuration, "EarthHammer");
}

void ArthurEarthHammer::goToRecover()
{
    if (!m_attackConfig || !m_animation)
    {
        return;
    }

    if (m_arthurController)
    {
        m_arthurController->setRecoveryDuration(m_attackConfig->m_earthHammerRecoveryDuration);
    }

    Debug::log("[ArthurEarthHammer] Going to Recover.");

    AnimationAPI::sendTrigger(m_animation, "ToRecover");
}

IMPLEMENT_SCRIPT(ArthurEarthHammer)