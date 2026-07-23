#include "pch.h"
#include "EnemyShadowMark.h"
#include "ReaperGauge.h"
#include "PersistingPowerupState.h"
#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(EnemyShadowMark, 
    SERIALIZED_BOOL(m_useMarkDuration, "Use Mark Duration"),
    SERIALIZED_FLOAT(m_markDuration, "Mark Duration", 0.5f, 10.0f, 0.1f),
    SERIALIZED_FLOAT(m_markFadeDuration, "Mark Fade Duration", 0.0f, 5.0f, 0.1f),
	SERIALIZED_COMPONENT_REF(m_canvas, "Canvas Transform", ComponentType::TRANSFORM2D),
	SERIALIZED_COMPONENT_REF(m_mark_death, "Mark Death Sprite", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_mark_lyriel, "Mark Lyriel Sprite", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_mark_both, "Mark Both Sprite", ComponentType::TRANSFORM),
    FIELD_GROUP_COLLAPSE("Effects",
        FIELD_GROUP_LABEL("Final Mark Explosion Effect"),
        SERIALIZED_FLOAT(m_explosionDuration, "Explosion Duration", 0.05f, 1.0f, 0.05f),
        SERIALIZED_FLOAT(m_explosionScaleMultiplier, "Explosion Scale Multiplier", 1.0f, 3.0f, 0.1f),
        FIELD_GROUP_LABEL("Mark Entry Pop Effect"),
        SERIALIZED_FLOAT(m_entryPopDuration, "Entry Pop Duration", 0.05f, 0.5f, 0.01f),
        SERIALIZED_FLOAT(m_entryPopStartScaleMultiplier, "Entry Pop Start Scale", 0.1f, 1.0f, 0.05f),
        SERIALIZED_FLOAT(m_entryPopPeakScaleMultiplier, "Entry Pop Peak Scale", 1.0f, 2.0f, 0.05f),
        SERIALIZED_FLOAT(m_readyPopPeakScaleMultiplier, "Ready Pop Peak Scale", 1.0f, 2.0f, 0.05f)
    )
)

EnemyShadowMark::EnemyShadowMark(GameObject* owner)
    : Script(owner)
{
}

void EnemyShadowMark::Start()
{
	m_canvasTransform2D = m_canvas.getReferencedComponent();
	m_mark1Object = m_mark_death.getReferencedComponent() ? ComponentAPI::getOwner(m_mark_death.getReferencedComponent()) : nullptr;
	m_mark2Object = m_mark_lyriel.getReferencedComponent() ? ComponentAPI::getOwner(m_mark_lyriel.getReferencedComponent()) : nullptr;
	m_mark3Object = m_mark_both.getReferencedComponent() ? ComponentAPI::getOwner(m_mark_both.getReferencedComponent()) : nullptr;

	if (!m_canvasTransform2D || !m_mark1Object || !m_mark2Object || !m_mark3Object)
	{
		Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
		Transform* shadowMarkTransform = TransformAPI::findChildByName(ownerTransform, "Shadow Mark");
		if (shadowMarkTransform)
		{
			GameObject* shadowMarkObject = ComponentAPI::getOwner(shadowMarkTransform);
			if (!m_canvasTransform2D)
			{
				m_canvasTransform2D = static_cast<Transform2D*>(GameObjectAPI::getComponent(shadowMarkObject, ComponentType::TRANSFORM2D));
			}

			if (!m_mark1Object)
			{
				Transform* mark1 = TransformAPI::findChildByName(shadowMarkTransform, "Shadow Mark Death");
				if (mark1) m_mark1Object = ComponentAPI::getOwner(mark1);
			}
			if (!m_mark2Object)
			{
				Transform* mark2 = TransformAPI::findChildByName(shadowMarkTransform, "Shadow Mark Lyriel");
				if (mark2) m_mark2Object = ComponentAPI::getOwner(mark2);
			}
			if (!m_mark3Object)
			{
				Transform* mark3 = TransformAPI::findChildByName(shadowMarkTransform, "Shadow Mark Both");
				if (mark3) m_mark3Object = ComponentAPI::getOwner(mark3);
			}
		}
	}

    if (m_canvasTransform2D)
    {
        m_originalScale = Transform2DAPI::getScale(m_canvasTransform2D);
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

    if (!m_useMarkDuration)
    {
        return;
    }

    m_timer -= Time::getDeltaTime();

    if (m_timer <= 0.0f)
    {
        resetMark();
        return;
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
    if (m_mark1Object)
    {
        GameObjectAPI::setActive(m_mark1Object, m_state == ShadowMarkState::DeathOnly);
    }

    if (m_mark2Object)
    {
        GameObjectAPI::setActive(m_mark2Object, m_state == ShadowMarkState::LyrielOnly);
    }

    if (m_mark3Object)
    {
        GameObjectAPI::setActive(m_mark3Object, m_state == ShadowMarkState::Ready);
    }

    if (!m_canvasTransform2D || m_isExploding || m_isEntryPopping)
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

    Transform2DAPI::setAlpha(m_canvasTransform2D, alpha);
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
        return true;

    case PlayerAttackType::DeathTaunt:
        return PersistingPowerupState::isUnlocked(PowerupId::DeathPowerup1);

    case PlayerAttackType::LyrielVolley:
        return PersistingPowerupState::isUnlocked(PowerupId::LyrielPowerup1);

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
        return true;

    case PlayerAttackType::DeathTaunt:
        return PersistingPowerupState::isUnlocked(PowerupId::DeathPowerup1);

    case PlayerAttackType::LyrielVolley:
        return PersistingPowerupState::isUnlocked(PowerupId::LyrielPowerup1);

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

    m_isExploding = false;
    m_explosionTimer = 0.0f;

    m_isEntryPopping = false;
    m_entryPopTimer = 0.0f;

    restoreUIVisuals();
    updateUI();
}

void EnemyShadowMark::startExplosion()
{
    if (!m_canvasTransform2D)
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
    if (!m_canvasTransform2D)
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

    Transform2DAPI::setScale(m_canvasTransform2D, scale);
    Transform2DAPI::setAlpha(m_canvasTransform2D, alpha);

    if (t >= 1.0f)
    {
        m_isExploding = false;
        resetMark();
    }
}

void EnemyShadowMark::restoreUIVisuals()
{
    if (!m_canvasTransform2D)
    {
        return;
    }

    Transform2DAPI::setScale(m_canvasTransform2D, m_originalScale);
    Transform2DAPI::setAlpha(m_canvasTransform2D, 1.0f);
}

void EnemyShadowMark::startEntryPop()
{
    if (!m_canvasTransform2D || m_entryPopDuration <= 0.0f)
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

    Transform2DAPI::setScale(m_canvasTransform2D, startScale);
    Transform2DAPI::setAlpha(m_canvasTransform2D, 0.0f);
}

void EnemyShadowMark::updateEntryPop()
{
    if (!m_canvasTransform2D)
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

        Transform2DAPI::setAlpha(m_canvasTransform2D, firstHalfT);
    }
    else
    {
        float secondHalfT = (t - 0.5f) / 0.5f;
        secondHalfT = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, secondHalfT);

        const float multiplier = m_currentPopPeakMultiplier + (1.0f - m_currentPopPeakMultiplier) * secondHalfT;

        scale = { m_originalScale.x * multiplier, m_originalScale.y * multiplier };

        Transform2DAPI::setAlpha(m_canvasTransform2D, 1.0f);
    }

    Transform2DAPI::setScale(m_canvasTransform2D, scale);

    if (t >= 1.0f)
    {
        m_isEntryPopping = false;
        m_entryPopTimer = 0.0f;

        restoreUIVisuals();
        updateUI();
    }
}

IMPLEMENT_SCRIPT(EnemyShadowMark)
