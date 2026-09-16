#include "pch.h"
#include "ArthurEarthHammer.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "EnemyAttackExecutor.h"
#include "ArthurUI.h"
#include "ArthurSound.h"
#include "ArthurParticles.h"
#include "CameraShake.h"

ArthurEarthHammer::ArthurEarthHammer(GameObject* owner)
    : StateMachineScript(owner)
{
}

void ArthurEarthHammer::OnStateEnter()
{
    m_arthurController = GameObjectAPI::findScript<ArthurBossController>(getOwner());
    m_attackExecutor = GameObjectAPI::findScript<EnemyAttackExecutor>(getOwner());
    m_animation = AnimationAPI::getAnimationComponent(getOwner());
    m_arthurUI = GameObjectAPI::findScript<ArthurUI>(getOwner());
    m_arthurSound = GameObjectAPI::findScript<ArthurSound>(getOwner());
    m_arthurParticles = GameObjectAPI::findScript<ArthurParticles>(getOwner());

    GameObject* cameraObject = SceneAPI::getDefaultCameraGameObject();
    m_cameraShake = cameraObject ? GameObjectAPI::findScript<CameraShake>(cameraObject) : nullptr;

    m_stateTimer = 0.0f;
    m_hasAppliedImpact = false;

    if (!m_arthurController)
    {
        Debug::error("[ArthurEarthHammer] ArthurBossController not found.");
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

    if (m_arthurSound)
    {
        m_arthurSound->playHammerPreparing();   // wind-up
    }

    if (m_arthurParticles)
    {
        m_arthurParticles->startEarthHammerShockwave();
    }

    Debug::log("[ArthurEarthHammer] ENTER");
}

void ArthurEarthHammer::OnStateUpdate()
{
    if (!m_arthurController || !m_arthurController->m_attackConfig.get() || !m_attackExecutor || !m_animation)
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
        m_arthurUI->updateEarthHammerUI(m_stateTimer, m_hasAppliedImpact, m_arthurController->m_attackConfig.get()->m_earthHammerHitTime, m_arthurController->m_attackConfig.get()->m_earthHammerRecoveryDuration);
    }

    if (!m_hasAppliedImpact && m_stateTimer >= m_arthurController->m_attackConfig.get()->m_earthHammerHitTime)
    {
        applyImpact();
        m_hasAppliedImpact = true;
    }

    if (m_stateTimer >= m_arthurController->m_attackConfig.get()->m_earthHammerTotalDuration)
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

    if (m_arthurParticles)
    {
        m_arthurParticles->stopEarthHammerShockwave();
    }

    Debug::log("[ArthurEarthHammer] EXIT");
}

void ArthurEarthHammer::applyImpact()
{
    if (!m_arthurController || !m_attackExecutor || !m_arthurController->m_attackConfig.get())
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

    float damage = m_arthurController->m_attackConfig.get()->m_earthHammerDamage;
    float stunDuration = m_arthurController->m_attackConfig.get()->m_earthHammerStunDuration;

    if (isPhase2)
    {
        damage = m_arthurController->m_attackConfig.get()->m_earthHammerPhase2Damage;
        stunDuration = m_arthurController->m_attackConfig.get()->m_earthHammerPhase2StunDuration;
    }

    m_attackExecutor->applyDamageAndStunInRadius(center, m_arthurController->m_attackConfig.get()->m_earthHammerRadius, damage, stunDuration, "EarthHammer");

    if (m_arthurSound)
    {
        m_arthurSound->playHammerImpact();
    }

    if (m_arthurParticles)
    {
        m_arthurParticles->playEarthHammerImpact(center);
    }

    if (m_cameraShake)
    {
        m_cameraShake->shakeImpact();
    }
}

void ArthurEarthHammer::goToRecover()
{
    if (!m_arthurController->m_attackConfig.get() || !m_animation)
    {
        return;
    }

    if (m_arthurController)
    {
        m_arthurController->setRecoveryDuration(m_arthurController->m_attackConfig.get()->m_earthHammerRecoveryDuration);
    }

    Debug::log("[ArthurEarthHammer] Going to Recover.");

    AnimationAPI::sendTrigger(m_animation, "ToRecover");
}

IMPLEMENT_SCRIPT(ArthurEarthHammer)