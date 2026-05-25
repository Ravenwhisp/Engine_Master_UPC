#include "pch.h"
#include "ArthurEarthHammer.h"

#include "ArthurBossController.h"
#include "ArthurAttackConfig.h"
#include "EnemyAttackExecutor.h"

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

    m_arthurController->clearPath();
    m_arthurController->updateCurrentTarget();
    m_arthurController->faceCurrentTarget();

    setupUI();

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

    updateUI();

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
    if (m_attackConfig && m_attackConfig->m_earthHammerUICanvasTransform)
    {
        GameObjectAPI::setActive(m_attackConfig->m_earthHammerUICanvasTransform->getOwner(), false);
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

void ArthurEarthHammer::setupUI()
{
    if (!m_attackConfig)
    {
        return;
    }

    Transform* canvas = m_attackConfig->m_earthHammerUICanvasTransform;
    Transform2D* container = m_attackConfig->m_earthHammerUIContainerTransform2D;
    Transform2D* ring = m_attackConfig->m_earthHammerUIRingTransform2D;
    Transform2D* inner = m_attackConfig->m_earthHammerUIInnerTransform2D;
    Transform2D* spikes = m_attackConfig->m_earthHammerUISpikesTransform2D;
    Transform2D* glow = m_attackConfig->m_earthHammerUIGlowTransform2D;

    if (!canvas || !container || !ring || !inner || !spikes || !glow)
    {
        return;
    }

    m_hasStartedImpactUI = false;
    m_impactUITimer = 0.0f;
    m_innerScale = 0.1f;

    GameObjectAPI::setActive(canvas->getOwner(), true);

    Transform2DAPI::setAlpha(ring, 1.0f);
    Transform2DAPI::setAlpha(inner, 1.0f);
    Transform2DAPI::setAlpha(spikes, 0.0f);
    Transform2DAPI::setAlpha(glow, 0.0f);
    Transform2DAPI::setAlpha(container, 0.0f);
}

void ArthurEarthHammer::updateUI()
{
    if (!m_attackConfig)
    {
        return;
    }


    Transform2D* container = m_attackConfig->m_earthHammerUIContainerTransform2D;
    Transform2D* ring = m_attackConfig->m_earthHammerUIRingTransform2D;
    Transform2D* inner = m_attackConfig->m_earthHammerUIInnerTransform2D;
	Transform2D* spikes = m_attackConfig->m_earthHammerUISpikesTransform2D;
    Transform2D* glow = m_attackConfig->m_earthHammerUIGlowTransform2D;

    if (!container || !ring || !inner || !spikes || !glow)
    {
        return;
    }

    const float hitTime = m_attackConfig->m_earthHammerHitTime;
    const float totalTime = m_attackConfig->m_earthHammerTotalDuration;

    // CHARGE PHASE

    if (!m_hasAppliedImpact)
    {
        const float t = std::clamp(m_stateTimer / hitTime, 0.0f, 1.0f);
        
        const float ringAlpha = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);
        Transform2DAPI::setAlpha(container, ringAlpha);

        m_innerScale = 0.1f + (t * 0.9f);
        Transform2DAPI::setScale(inner, Vector2(m_innerScale, m_innerScale));

        return;
    }

    // IMPACT PHASE

    if (!m_hasStartedImpactUI)
    {
        m_hasStartedImpactUI = true;
        m_impactUITimer = 0.0f;
        m_innerScale = 1.0f;
    }

    const float dt = Time::getDeltaTime();
    m_impactUITimer += dt;

    const float impactDuration = m_attackConfig->m_earthHammerRecoveryDuration;
    const float t = std::clamp(m_impactUITimer / impactDuration, 0.0f, 1.0f);


    const float containerAlpha = 1.0f - MathAPI::evaluateEasing(MathAPI::EasingType::EaseInCubic, t);
    Transform2DAPI::setAlpha(container, containerAlpha);

    const float glowAlpha = 1.0f - MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutQuad, t);
    Transform2DAPI::setAlpha(glow, glowAlpha);
    Transform2DAPI::setAlpha(spikes, glowAlpha);
}

IMPLEMENT_SCRIPT(ArthurEarthHammer)