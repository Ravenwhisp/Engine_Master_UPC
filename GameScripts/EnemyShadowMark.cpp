#include "pch.h"
#include "EnemyShadowMark.h"
#include "ReaperGauge.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(EnemyShadowMark, 
    SERIALIZED_FLOAT(m_markDuration, "Mark Duration", 0.5f, 10.0f, 0.1f),
	SERIALIZED_FLOAT(m_markUITargetScale, "Mark UI Scale", 0.1f, 5.0f, 0.2f),
	SERIALIZED_FLOAT(m_markUIHeightOffset, "Mark UI Height", 0.1f, 50.0f, 20.0f),
	SERIALIZED_COMPONENT_REF(m_canvas, "Canvas Transform", ComponentType::TRANSFORM2D),
	SERIALIZED_COMPONENT_REF(m_mark_1, "Mark Phase 1", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_mark_2, "Mark Phase 2", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_mark_3, "Mark Phase 3", ComponentType::TRANSFORM)
)

EnemyShadowMark::EnemyShadowMark(GameObject* owner)
    : Script(owner)
{
}

void EnemyShadowMark::Start()
{
	m_canvasTransform2D = m_canvas.getReferencedComponent();
    if (m_canvasTransform2D)
    {
		m_startScale = Transform2DAPI::getScale(m_canvasTransform2D).x;
	}
	m_mark1Object = m_mark_1.getReferencedComponent() ? ComponentAPI::getOwner(m_mark_1.getReferencedComponent()) : nullptr;
	m_mark2Object = m_mark_2.getReferencedComponent() ? ComponentAPI::getOwner(m_mark_2.getReferencedComponent()) : nullptr;
	m_mark3Object = m_mark_3.getReferencedComponent() ? ComponentAPI::getOwner(m_mark_3.getReferencedComponent()) : nullptr;
    updateUI();
}

void EnemyShadowMark::Update()
{
    updateUI();

    if (m_phase == 0)
    {
        return;
    }

    m_timer -= Time::getDeltaTime();
    if (m_timer <= 0.0f)
    {
        m_phase = 0;
        m_timer = 0.0f;
        Debug::log("[ShadowMark] Mark expired.");
    }
}

void EnemyShadowMark::notifyDeathHit()
{
    if (m_phase < 3)
    {
        m_phase++;
    }

    m_timer = m_markDuration;
    Debug::log("[ShadowMark] Phase %d  timer reset.", m_phase);
}

void EnemyShadowMark::exploit()
{
    Debug::log("[ShadowMark] Mark exploited at phase %d!", m_phase);

    if (m_reaperGauge == nullptr)
        m_reaperGauge = findReaperGauge();

    if (m_reaperGauge != nullptr)
        m_reaperGauge->onMarkExploited();
    else
        Debug::warn("[ShadowMark] ReaperGauge not found on any GameObject. Make sure GameController has a ReaperGauge script.");

    m_phase = 0;
    m_timer = 0.0f;
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
        GameObjectAPI::setActive(m_mark1Object, m_phase == 1);
    }
    if (m_mark2Object)
    {
        GameObjectAPI::setActive(m_mark2Object, m_phase == 2);
    }
    if (m_mark3Object)
    {
        GameObjectAPI::setActive(m_mark3Object, m_phase == 3);
    }
    
	if (m_timer <= 0.0f)
    {
        return;
    }

    if (m_canvasTransform2D)
    {
        const float t = (m_markDuration - m_timer) / m_markDuration;
        const float easedTimerPos = MathAPI::evaluateEasing(MathAPI::EasingType::EaseOutCubic, t);
        Transform2DAPI::setPosition(m_canvasTransform2D, { 0.0f, easedTimerPos * m_markUIHeightOffset });

        const float easedTimerScale = MathAPI::evaluateEasing(MathAPI::EasingType::EaseInSine, t);
		const float scale = m_startScale + (m_markUITargetScale - m_startScale) * easedTimerScale;
        Transform2DAPI::setScale(m_canvasTransform2D, { scale, scale });
    }
}

void EnemyShadowMark::drawGizmo()
{
    if (m_phase == 0)
        return;

    const Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t == nullptr)
        return;

    Vector3 pos = TransformAPI::getGlobalPosition(t);
    pos.y += 1.8f;

    float   radius;
    Vector3 color;
    switch (m_phase)
    {
    case 1:  radius = 0.20f; color = { 0.35f, 0.35f, 0.35f }; break;
    case 2:  radius = 0.35f; color = { 0.85f, 0.40f, 0.00f }; break;
    default: radius = 0.50f; color = { 0.00f, 0.55f, 1.00f }; break;
    }

    DebugDrawAPI::drawSphere(pos, color, radius, 0, true);

    // Timer ring: white partial arc in XZ plane that shrinks as timer runs out
    if (m_markDuration > 0.0f)
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

IMPLEMENT_SCRIPT(EnemyShadowMark)
