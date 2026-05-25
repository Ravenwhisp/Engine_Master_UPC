#include "pch.h"
#include "ReaperGauge.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(ReaperGauge,
    SERIALIZED_FLOAT(m_maxGauge, "Max Gauge", 1.0f, 500.0f, 1.0f),
    SERIALIZED_INT(m_numSegments, "Num Segments"),
    SERIALIZED_FLOAT(m_gainPerExploit, "Gain Per Exploit", 0.0f, 100.0f, 1.0f),
    SERIALIZED_FLOAT(m_gracePeriod, "Grace Period", 0.0f, 60.0f, 0.5f),
    SERIALIZED_FLOAT(m_decayPerSecond, "Decay Per Second", 0.0f, 50.0f, 0.5f),
	SERIALIZED_COMPONENT_REF(m_reaperGaugeUI, "Reaper Gauge UI", ComponentType::UISLIDER),
    SERIALIZED_COMPONENT_REF(m_glowUI, "Glow UI", ComponentType::TRANSFORM2D),
    SERIALIZED_COMPONENT_REF(m_blinkAlphaUI, "Blink Alpha UI", ComponentType::TRANSFORM2D),
    SERIALIZED_FLOAT(m_blinkSpeed, "Blink Speed", 0.1f, 20.0f, 0.1f),
	SERIALIZED_FLOAT(m_blinkAlpha, "Blink Alpha", 0.0f, 1.0f, 0.05f)
)

ReaperGauge::ReaperGauge(GameObject* owner)
    : Script(owner)
{
}

void ReaperGauge::Start()
{
	m_reaperGaugeSlider = m_reaperGaugeUI.getReferencedComponent();
	m_glowTransform = m_glowUI.getReferencedComponent();
	m_blinkAlphaTransform = m_blinkAlphaUI.getReferencedComponent();

    SliderAPI::setFillAmount(m_reaperGaugeSlider, getGaugePercent());
    Transform2DAPI::setAlpha(m_glowTransform, 0.0f);
    Transform2DAPI::setAlpha(m_blinkAlphaTransform, 0.0f);
}

void ReaperGauge::Update()
{
    if (!m_everExploited || m_gauge <= 0.0f)
        return;

    m_decayTimer += Time::getDeltaTime();

    if (m_decayTimer > m_gracePeriod)
    {
        const bool wasAboveZero = m_gauge > 0.0f;

        if (!m_decaying)
        {
            m_decaying = true;
            Debug::log("[ReaperGauge] Grace period ended. Gauge decaying: %.1f/%.1f", m_gauge, m_maxGauge);
        }

        m_gauge -= m_decayPerSecond * Time::getDeltaTime();
        if (m_gauge < 0.0f)
            m_gauge = 0.0f;

        if (wasAboveZero && m_gauge <= 0.0f)
        {
            m_decaying = false;
            Debug::log("[ReaperGauge] Gauge empty. Shadow Execution NOT available.");
        }
    }
    updateUI();
}

void ReaperGauge::onMarkExploited()
{
    m_everExploited = true;
    m_decayTimer    = 0.0f;
    m_decaying      = false;

    const bool wasFull = isFull();

    m_gauge += m_gainPerExploit;
    if (m_gauge > m_maxGauge)
        m_gauge = m_maxGauge;

    Debug::log("[ReaperGauge] Mark exploited! +%.1f%%  =>  %.1f%%  (%d/%d segments)",
        (m_gainPerExploit / m_maxGauge) * 100.0f, getGaugePercent() * 100.0f,
        getCurrentSegments(), m_numSegments);

    if (!wasFull && isFull())
        Debug::log("[ReaperGauge] GAUGE FULL! Shadow Execution is now available.");
}

void ReaperGauge::consume()
{
    m_gauge      = 0.0f;
    m_decayTimer = 0.0f;
    m_decaying   = false;
    Debug::log("[ReaperGauge] Gauge consumed by Shadow Execution.");
}

float ReaperGauge::getGaugePercent() const
{
    if (m_maxGauge <= 0.0f)
        return 0.0f;
    return m_gauge / m_maxGauge;
}

int ReaperGauge::getCurrentSegments() const
{
    if (m_maxGauge <= 0.0f || m_numSegments <= 0)
        return 0;
    const float segValue = m_maxGauge / static_cast<float>(m_numSegments);
    return static_cast<int>(m_gauge / segValue);
}

void ReaperGauge::updateUI()
{
    if (m_reaperGaugeSlider)
    {
        SliderAPI::setFillAmount(m_reaperGaugeSlider, getGaugePercent());
    }

    if (m_glowTransform)
    {
        if (m_decayTimer <= m_gracePeriod)
        {
            const float alpha = 1.0f - (m_decayTimer / m_gracePeriod);
            Transform2DAPI::setAlpha(m_glowTransform, alpha);
        }
    }

    if (m_blinkAlphaTransform)
    {
        if (m_decayTimer > m_gracePeriod)
        {
			const float t = (sinf((m_decayTimer - m_gracePeriod) * m_blinkSpeed) + 1.0f) * 0.5f;
            Transform2DAPI::setAlpha(m_blinkAlphaTransform, t * m_blinkAlpha);
        }
        else
        {
            Transform2DAPI::setAlpha(m_blinkAlphaTransform, 0.0f);
        }
    }
}

void ReaperGauge::drawGizmo()
{
    const Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t == nullptr)
        return;

    const Vector3 pos = TransformAPI::getGlobalPosition(t);

    // Segmented bar flat in XZ plane, centred on Lyriel, slightly below feet
    const float groundY = pos.y + 0.05f;
    const float segW    = 0.30f;
    const float segD    = 0.12f; // depth along Z
    const float padding = 0.05f;
    const int   segs    = m_numSegments > 0 ? m_numSegments : 1;
    const float total   = segs * segW + (segs - 1) * padding;
    const float startX  = pos.x - total * 0.5f;

    const float gaugePercent = getGaugePercent();
    const float filled       = gaugePercent * static_cast<float>(segs);

    const Vector3 colFilled = { 0.85f, 0.10f, 0.10f };
    const Vector3 colEmpty  = { 0.30f, 0.30f, 0.30f };

    for (int i = 0; i < segs; ++i)
    {
        const float x0 = startX + i * (segW + padding);
        const float x1 = x0 + segW;
        const float z0 = pos.z - segD * 0.5f;
        const float z1 = pos.z + segD * 0.5f;

        const float segFill = filled - static_cast<float>(i);
        const float ratio   = segFill < 0.0f ? 0.0f : (segFill > 1.0f ? 1.0f : segFill);
        const Vector3 col   = ratio > 0.0f ? colFilled : colEmpty;

        // Outline rectangle in XZ
        DebugDrawAPI::drawLine({ x0, groundY, z0 }, { x1, groundY, z0 }, col, 0, true);
        DebugDrawAPI::drawLine({ x0, groundY, z1 }, { x1, groundY, z1 }, col, 0, true);
        DebugDrawAPI::drawLine({ x0, groundY, z0 }, { x0, groundY, z1 }, col, 0, true);
        DebugDrawAPI::drawLine({ x1, groundY, z0 }, { x1, groundY, z1 }, col, 0, true);

        // Fill: diagonal cross-lines inside the filled portion of the segment
        if (ratio > 0.0f)
        {
            const float fillX = x0 + segW * ratio;
            DebugDrawAPI::drawLine({ x0,    groundY, z0 }, { fillX, groundY, z1 }, colFilled, 0, true);
            DebugDrawAPI::drawLine({ x0,    groundY, z1 }, { fillX, groundY, z0 }, colFilled, 0, true);
        }
    }

    // Decay timer arc around bar centre — white=grace, red=decaying
    if (m_everExploited && m_gauge > 0.0f)
    {
        const bool  inGrace = m_decayTimer <= m_gracePeriod;
        const float ratio   = inGrace
            ? 1.0f - (m_decayTimer / m_gracePeriod)
            : 0.0f;

        const Vector3 arcColor = inGrace ? Vector3{ 1.0f, 1.0f, 1.0f } : Vector3{ 1.0f, 0.2f, 0.2f };
        const float   arcR     = total * 0.5f + 0.2f;
        const int     totalSeg = 24;
        const int     fillSeg  = inGrace
            ? static_cast<int>(ratio * static_cast<float>(totalSeg))
            : totalSeg;
        constexpr float pi2    = 2.0f * 3.14159265f;
        const float     step   = pi2 / static_cast<float>(totalSeg);

        for (int i = 0; i < fillSeg; ++i)
        {
            const float   a0 = step * static_cast<float>(i);
            const float   a1 = a0 + step;
            const Vector3 p0 = { pos.x + cosf(a0) * arcR, groundY, pos.z + sinf(a0) * arcR };
            const Vector3 p1 = { pos.x + cosf(a1) * arcR, groundY, pos.z + sinf(a1) * arcR };
            DebugDrawAPI::drawLine(p0, p1, arcColor, 0, true);
        }
    }
}

IMPLEMENT_SCRIPT(ReaperGauge)
