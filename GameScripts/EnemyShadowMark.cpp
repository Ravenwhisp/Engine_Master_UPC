#include "pch.h"
#include "EnemyShadowMark.h"
#include "ReaperGauge.h"
#include "PersistingPowerupState.h"
#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(EnemyShadowMark, 
    SERIALIZED_BOOL(m_useMarkDuration, "Use Mark Duration"),
    SERIALIZED_FLOAT(m_markDuration, "Mark Duration", 0.5f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_markFadeDuration, "Mark Fade Duration", 0.0f, 5.0f, 0.1f),
    SERIALIZED_COMPONENT_REF(m_markContainer, "Mark Container Transform", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(m_backgroundGlow, "Background Glow", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(m_backgroundBlur, "Background Blur", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(m_deathFragment, "Death Fragment", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(m_lyrielFragment, "Lyriel Fragment", ComponentType::TRANSFORM2D),
    FIELD_GROUP_COLLAPSE("Effects",
        FIELD_GROUP_LABEL("Final Mark Explosion Effect"),
        SERIALIZED_FLOAT(m_explosionDuration, "Explosion Duration", 0.05f, 1.0f, 0.05f),
        SERIALIZED_FLOAT(m_explosionScaleMultiplier, "Explosion Scale Multiplier", 1.0f, 3.0f, 0.1f),
        FIELD_GROUP_LABEL("Mark Entry Pop Effect"),
        SERIALIZED_FLOAT(m_entryPopDuration, "Entry Pop Duration", 0.05f, 0.5f, 0.01f),
        SERIALIZED_FLOAT(m_entryPopStartScaleMultiplier, "Entry Pop Start Scale", 0.1f, 1.0f, 0.05f),
        SERIALIZED_FLOAT(m_entryPopPeakScaleMultiplier, "Entry Pop Peak Scale", 1.0f, 2.0f, 0.05f),
        SERIALIZED_FLOAT(m_readyPopPeakScaleMultiplier, "Ready Pop Peak Scale", 1.0f, 2.0f, 0.05f),
        FIELD_GROUP_LABEL("Inactive Fragment Pulse"),
        SERIALIZED_FLOAT(m_inactivePulseMinAlpha, "Min Alpha", 0.0f, 1.0f, 0.05f),
        SERIALIZED_FLOAT(m_inactivePulseMaxAlpha, "Max Alpha", 0.0f, 1.0f, 0.05f),
        SERIALIZED_FLOAT(m_inactivePulseFrequency, "Frequency", 0.1f, 5.0f, 0.1f)
    )
)

EnemyShadowMark::EnemyShadowMark(GameObject* owner)
    : Script(owner)
{
}

void EnemyShadowMark::Start()
{
    m_markContainerTransform2D = m_markContainer.getReferencedComponent();
    m_deathFragmentTransform2D = m_deathFragment.getReferencedComponent();
    m_backgroundGlowTransform2D = m_backgroundGlow.getReferencedComponent();
    m_backgroundBlurTransform2D = m_backgroundBlur.getReferencedComponent();
    m_lyrielFragmentTransform2D = m_lyrielFragment.getReferencedComponent();

    if (!m_markContainerTransform2D || !m_backgroundGlowTransform2D || !m_backgroundBlurTransform2D || !m_deathFragmentTransform2D || !m_lyrielFragmentTransform2D)
    {
        Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
        Transform* healthBarTransform = TransformAPI::findChildByName(ownerTransform, "Health Bar");
        Transform* shadowMarkTransform = healthBarTransform ? TransformAPI::findChildByName(healthBarTransform, "Shadow Mark") : nullptr;

        if (shadowMarkTransform)
        {
            GameObject* shadowMarkObject = ComponentAPI::getOwner(shadowMarkTransform);

            if (!m_markContainerTransform2D)
            {
                m_markContainerTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(shadowMarkObject, ComponentType::TRANSFORM2D));
            }

            if (!m_deathFragmentTransform2D)
            {
                Transform* deathFragment = TransformAPI::findChildByName(shadowMarkTransform, "Death Fragment");

                if (deathFragment)
                {
                    GameObject* deathObject = ComponentAPI::getOwner(deathFragment);
                    m_deathFragmentTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(deathObject, ComponentType::TRANSFORM2D));
                }
            }

            if (!m_lyrielFragmentTransform2D)
            {
                Transform* lyrielFragment = TransformAPI::findChildByName(shadowMarkTransform, "Lyriel Fragment");

                if (lyrielFragment)
                {
                    GameObject* lyrielObject = ComponentAPI::getOwner(lyrielFragment);
                    m_lyrielFragmentTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(lyrielObject, ComponentType::TRANSFORM2D));
                }
            }

            if (!m_backgroundGlowTransform2D)
            {
                Transform* backgroundGlow = TransformAPI::findChildByName(shadowMarkTransform, "Background Glow");

                if (backgroundGlow)
                {
                    GameObject* backgroundGlowObject = ComponentAPI::getOwner(backgroundGlow);
                    m_backgroundGlowTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(backgroundGlowObject, ComponentType::TRANSFORM2D));
                }
            }

            if (!m_backgroundBlurTransform2D)
            {
                Transform* backgroundBlur = TransformAPI::findChildByName(shadowMarkTransform, "Background Blur");

                if (backgroundBlur)
                {
                    GameObject* backgroundBlurObject = ComponentAPI::getOwner(backgroundBlur);
                    m_backgroundBlurTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(backgroundBlurObject, ComponentType::TRANSFORM2D));
                }
            }
        }
    }

    if (m_markContainerTransform2D)
    {
        m_originalScale = Transform2DAPI::getScale(m_markContainerTransform2D);
    }

    updateUI();
}

void EnemyShadowMark::Update()
{
    if (m_isExploding)
    {
        updateExplosion();
        return;
    }

    if (m_isEntryPopping)
    {
        updateEntryPop();
        return;
    }

    if (m_state == ShadowMarkState::None)
    {
        return;
    }

    m_pulseTimer += Time::getDeltaTime();

    if (m_useMarkDuration)
    {
        m_timer -= Time::getDeltaTime();

        if (m_timer <= 0.0f)
        {
            resetMark();
            return;
        }
    }

    updateUI();
}

bool EnemyShadowMark::processAttack(PlayerAttackType attackType)
{
    if (m_isExploding)
    {
        return false;
    }

    if (m_state == ShadowMarkState::Ready && canExploitWith(attackType))
    {
        exploit();
        return true;
    }

    if (!canApplyWith(attackType))
    {
        return false;
    }

    if (isDeathAttack(attackType))
    {
        applyDeathContribution();
        return false;
    }

    if (isLyrielAttack(attackType))
    {
        applyLyrielContribution();
    }

    return false;
}

void EnemyShadowMark::exploit()
{
    Debug::log("[ShadowMark] Mark exploited");

    if (m_reaperGauge == nullptr)
        m_reaperGauge = findReaperGauge();

    if (m_reaperGauge != nullptr)
        m_reaperGauge->onMarkExploited();
    else
        Debug::warn("[ShadowMark] ReaperGauge not found on any GameObject. Make sure GameController has a ReaperGauge script.");

    startExplosion();
}

ReaperGauge* EnemyShadowMark::findReaperGauge()
{
    const std::vector<GameObject*> holders = SceneAPI::findAllGameObjectsWithScript<ReaperGauge>();
    if (holders.empty())
        return nullptr;
    return GameObjectAPI::findScript<ReaperGauge>(holders[0]);
}

void EnemyShadowMark::updateUI()
{
    const float pulseNormalized = (1.0f - cosf(m_pulseTimer * m_inactivePulseFrequency * 2.0f * MathAPI::PI)) * 0.5f;
    const float pulseAlpha = m_inactivePulseMinAlpha + (m_inactivePulseMaxAlpha - m_inactivePulseMinAlpha) * pulseNormalized;

    float deathAlpha = 0.0f;
    float lyrielAlpha = 0.0f;
    float backgroundGlowAlpha = 0.0f;
    float backgroundBlurAlpha = 0.0f;

    switch (m_state)
    {
    case ShadowMarkState::None:
        break;

    case ShadowMarkState::DeathOnly:
        deathAlpha = 1.0f;
        lyrielAlpha = pulseAlpha;
        backgroundGlowAlpha = 1.0f;
        break;

    case ShadowMarkState::LyrielOnly:
        deathAlpha = pulseAlpha;
        lyrielAlpha = 1.0f;
        backgroundGlowAlpha = 1.0f;
        break;

    case ShadowMarkState::Ready:
        deathAlpha = 1.0f;
        lyrielAlpha = 1.0f;
        backgroundGlowAlpha = 1.0f;
        backgroundBlurAlpha = 1.0f;
        break;
    }

    if (m_deathFragmentTransform2D)
    {
        Transform2DAPI::setAlpha(m_deathFragmentTransform2D, deathAlpha);
    }

    if (m_lyrielFragmentTransform2D)
    {
        Transform2DAPI::setAlpha(m_lyrielFragmentTransform2D, lyrielAlpha);
    }

    if (m_backgroundGlowTransform2D)
    {
        Transform2DAPI::setAlpha(m_backgroundGlowTransform2D, backgroundGlowAlpha);
    }

    if (m_backgroundBlurTransform2D)
    {
        Transform2DAPI::setAlpha(m_backgroundBlurTransform2D, backgroundBlurAlpha);
    }

    if (!m_markContainerTransform2D || m_isExploding || m_isEntryPopping)
    {
        return;
    }

    float alpha = 1.0f;

    if (m_state != ShadowMarkState::None && m_useMarkDuration && m_markFadeDuration > 0.0f && m_markDuration > 0.0f)
    {
        const float fadeDuration = (std::min)(m_markFadeDuration, m_markDuration);

        if (m_timer <= fadeDuration)
        {
            float fadeProgress = 1.0f - (m_timer / fadeDuration);
            fadeProgress = std::clamp(fadeProgress, 0.0f, 1.0f);

            const float easedProgress = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInSine, fadeProgress);
            alpha = 1.0f - easedProgress;
        }
    }

    Transform2DAPI::setAlpha(m_markContainerTransform2D, alpha);
}

void EnemyShadowMark::drawGizmo()
{
    if (m_state == ShadowMarkState::None)
    {
        return;
    }

    const Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t == nullptr)
        return;

    Vector3 pos = TransformAPI::getGlobalPosition(t);
    pos.y += 1.8f;

    float   radius;
    Vector3 color;
    switch (m_state)
    {
    case ShadowMarkState::DeathOnly:
        radius = 0.20f;
        color = { 0.35f, 0.35f, 0.35f };
        break;

    case ShadowMarkState::LyrielOnly:
        radius = 0.35f;
        color = { 0.85f, 0.40f, 0.00f };
        break;

    case ShadowMarkState::Ready:
        radius = 0.50f;
        color = { 0.00f, 0.55f, 1.00f };
        break;
    }

    DebugDrawAPI::drawSphere(pos, color, radius, 0, true);

    // Timer ring: white partial arc in XZ plane that shrinks as timer runs out
    if (m_useMarkDuration &&  m_markDuration > 0.0f)
    {
        const float    ratio    = m_timer / m_markDuration;
        const float    ringR    = radius + 0.15f;
        const int      totalSeg = 24;
        const int      fillSeg  = static_cast<int>(ratio * static_cast<float>(totalSeg));
        constexpr float pi2     = 2.0f * 3.14159265f;
        const float    step     = pi2 / static_cast<float>(totalSeg);
        const Vector3  white    = { 1.0f, 1.0f, 1.0f };

        for (int i = 0; i < fillSeg; ++i)
        {
            const float   a0 = step * static_cast<float>(i);
            const float   a1 = a0 + step;
            const Vector3 p0 = { pos.x + cosf(a0) * ringR, pos.y, pos.z + sinf(a0) * ringR };
            const Vector3 p1 = { pos.x + cosf(a1) * ringR, pos.y, pos.z + sinf(a1) * ringR };
            DebugDrawAPI::drawLine(p0, p1, white, 0, true);
        }
    }
}

bool EnemyShadowMark::isDeathAttack(PlayerAttackType attackType) const
{
    switch (attackType)
    {
    case PlayerAttackType::DeathBasic:
    case PlayerAttackType::DeathCharged:
    case PlayerAttackType::DeathDash:
    case PlayerAttackType::DeathTaunt:
        return true;

    default:
        return false;
    }
}

bool EnemyShadowMark::isLyrielAttack(PlayerAttackType attackType) const
{
    switch (attackType)
    {
    case PlayerAttackType::LyrielArrow:
    case PlayerAttackType::LyrielVolley:
    case PlayerAttackType::LyrielCharged:
        return true;

    default:
        return false;
    }
}

bool EnemyShadowMark::canApplyWith(PlayerAttackType attackType) const
{
    switch (attackType)
    {
    case PlayerAttackType::DeathBasic:
    case PlayerAttackType::DeathCharged:
    case PlayerAttackType::DeathDash:
    case PlayerAttackType::LyrielArrow:
    case PlayerAttackType::LyrielCharged:
    case PlayerAttackType::DeathTaunt:
    case PlayerAttackType::LyrielVolley:
        return true;

    /*case PlayerAttackType::DeathTaunt:
        return PersistingPowerupState::isUnlocked(PowerupId::DeathPowerup1);

    case PlayerAttackType::LyrielVolley:
        return PersistingPowerupState::isUnlocked(PowerupId::LyrielPowerup1);*/

    default:
        return false;
    }
}

bool EnemyShadowMark::canExploitWith(PlayerAttackType attackType) const
{
    switch (attackType)
    {
    case PlayerAttackType::DeathCharged:
    case PlayerAttackType::LyrielCharged:
    case PlayerAttackType::DeathTaunt:
    case PlayerAttackType::LyrielVolley:
        return true;

    /*case PlayerAttackType::DeathTaunt:
        return PersistingPowerupState::isUnlocked(PowerupId::DeathPowerup1);

    case PlayerAttackType::LyrielVolley:
        return PersistingPowerupState::isUnlocked(PowerupId::LyrielPowerup1);*/

    default:
        return false;
    }
}

void EnemyShadowMark::applyDeathContribution()
{
    const ShadowMarkState previousState = m_state;

    switch (m_state)
    {
    case ShadowMarkState::None:
        m_state = ShadowMarkState::DeathOnly;
        break;

    case ShadowMarkState::DeathOnly:
        break;

    case ShadowMarkState::LyrielOnly:
        m_state = ShadowMarkState::Ready;
        break;

    case ShadowMarkState::Ready:
        break;
    }

    resetTimer();

    if (m_state != previousState)
    {
        m_pulseTimer = 0.0f;
        updateUI();
        startEntryPop();
    }
}

void EnemyShadowMark::applyLyrielContribution()
{
    const ShadowMarkState previousState = m_state;

    switch (m_state)
    {
    case ShadowMarkState::None:
        m_state = ShadowMarkState::LyrielOnly;
        break;

    case ShadowMarkState::DeathOnly:
        m_state = ShadowMarkState::Ready;
        break;

    case ShadowMarkState::LyrielOnly:
        break;

    case ShadowMarkState::Ready:
        break;
    }

    resetTimer();

    if (m_state != previousState)
    {
        m_pulseTimer = 0.0f;
        updateUI();
        startEntryPop();
    }
}

void EnemyShadowMark::resetTimer()
{
    m_timer = m_markDuration;

    if (!m_isEntryPopping && !m_isExploding)
    {
        restoreUIVisuals();
    }
}

void EnemyShadowMark::resetMark()
{
    m_state = ShadowMarkState::None;
    m_timer = 0.0f;
    m_pulseTimer = 0.0f;

    m_isExploding = false;
    m_explosionTimer = 0.0f;

    m_isEntryPopping = false;
    m_entryPopTimer = 0.0f;

    restoreUIVisuals();
    updateUI();
}

void EnemyShadowMark::startExplosion()
{
    if (!m_markContainerTransform2D)
    {
        resetMark();
        return;
    }

    if (m_explosionDuration <= 0.0f)
    {
        resetMark();
        return;
    }

    m_isExploding = true;
    m_explosionTimer = 0.0f;

    restoreUIVisuals();
    updateUI();
}

void EnemyShadowMark::updateExplosion()
{
    if (!m_markContainerTransform2D)
    {
        m_isExploding = false;
        resetMark();
        return;
    }

    m_explosionTimer += Time::getDeltaTime();

    float t = m_explosionTimer / m_explosionDuration;
    t = std::clamp(t, 0.0f, 1.0f);

    const float scaleProgress = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, t);

    const float targetScaleX = m_originalScale.x * m_explosionScaleMultiplier;
    const float targetScaleY = m_originalScale.y * m_explosionScaleMultiplier;

    const Vector2 scale = { m_originalScale.x + (targetScaleX - m_originalScale.x) * scaleProgress, m_originalScale.y + (targetScaleY - m_originalScale.y) * scaleProgress };

    const float fadeProgress = MathAPI::evaluateEasing( MathAPI::EasingType::EaseInSine, t);

    const float alpha = 1.0f - fadeProgress;

    Transform2DAPI::setScale(m_markContainerTransform2D, scale);
    Transform2DAPI::setAlpha(m_markContainerTransform2D, alpha);

    if (t >= 1.0f)
    {
        m_isExploding = false;
        resetMark();
    }
}

void EnemyShadowMark::restoreUIVisuals()
{
    if (!m_markContainerTransform2D)
    {
        return;
    }

    Transform2DAPI::setScale(m_markContainerTransform2D, m_originalScale);
    Transform2DAPI::setAlpha(m_markContainerTransform2D, 1.0f);
}

void EnemyShadowMark::startEntryPop()
{
    if (!m_markContainerTransform2D || m_entryPopDuration <= 0.0f)
    {
        restoreUIVisuals();
        updateUI();
        return;
    }

    m_isEntryPopping = true;
    m_entryPopTimer = 0.0f;

    if (m_state == ShadowMarkState::Ready)
    {
        m_currentPopPeakMultiplier = m_readyPopPeakScaleMultiplier;
    }
    else 
    {
        m_currentPopPeakMultiplier = m_entryPopPeakScaleMultiplier;
    }
                
    const Vector2 startScale = { m_originalScale.x * m_entryPopStartScaleMultiplier, m_originalScale.y * m_entryPopStartScaleMultiplier };

    Transform2DAPI::setScale(m_markContainerTransform2D, startScale);
    Transform2DAPI::setAlpha(m_markContainerTransform2D, 0.0f);
}

void EnemyShadowMark::updateEntryPop()
{
    if (!m_markContainerTransform2D)
    {
        m_isEntryPopping = false;
        return;
    }

    m_entryPopTimer += Time::getDeltaTime();

    float t = m_entryPopTimer / m_entryPopDuration;
    t = std::clamp(t, 0.0f, 1.0f);

    Vector2 scale = m_originalScale;

    if (t < 0.5f)
    {
        float firstHalfT = t / 0.5f;
        firstHalfT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, firstHalfT);

        const float multiplier = m_entryPopStartScaleMultiplier + (m_currentPopPeakMultiplier - m_entryPopStartScaleMultiplier) * firstHalfT;

        scale = { m_originalScale.x * multiplier, m_originalScale.y * multiplier };

        Transform2DAPI::setAlpha(m_markContainerTransform2D, firstHalfT);
    }
    else
    {
        float secondHalfT = (t - 0.5f) / 0.5f;
        secondHalfT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, secondHalfT);

        const float multiplier = m_currentPopPeakMultiplier + (1.0f - m_currentPopPeakMultiplier) * secondHalfT;

        scale = { m_originalScale.x * multiplier, m_originalScale.y * multiplier };

        Transform2DAPI::setAlpha(m_markContainerTransform2D, 1.0f);
    }

    Transform2DAPI::setScale(m_markContainerTransform2D, scale);

    if (t >= 1.0f)
    {
        m_isEntryPopping = false;
        m_entryPopTimer = 0.0f;

        restoreUIVisuals();
        updateUI();
    }
}

IMPLEMENT_SCRIPT(EnemyShadowMark)
