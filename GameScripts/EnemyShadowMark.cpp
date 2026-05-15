#include "pch.h"
#include "EnemyShadowMark.h"
#include "ReaperGauge.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(EnemyShadowMark, 
    SERIALIZED_FLOAT(m_markDuration, "Mark Duration", 0.5f, 10.0f, 0.1f)
)

EnemyShadowMark::EnemyShadowMark(GameObject* owner)
    : Script(owner)
{
}

void EnemyShadowMark::Start()
{
}

void EnemyShadowMark::Update()
{
    if (m_phase == 0)
        return;

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
        m_phase++;

    m_timer = m_markDuration;
    Debug::log("[ShadowMark] Phase %d  timer reset.", m_phase);
}

void EnemyShadowMark::exploit()
{
    Debug::log("[ShadowMark] Mark exploited at phase %d!", m_phase);

    std::vector<GameObject*> players = SceneAPI::findAllGameObjectsByTag(Tag::PLAYER, true);
    for (GameObject* player : players)
    {
        ReaperGauge* gauge = GameObjectAPI::findScript<ReaperGauge>(player);
        if (gauge != nullptr)
        {
            gauge->onMarkExploited();
            break;
        }
    }

    m_phase = 0;
    m_timer = 0.0f;
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
