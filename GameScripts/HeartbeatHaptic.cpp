#include "pch.h"
#include "HeartbeatHaptic.h"

IMPLEMENT_SCRIPT_FIELDS(HeartbeatHaptic,
    SERIALIZED_FLOAT(m_hapticIntensity, "Heartbeat Intensity", 100.0f, 0.0f, 0.01f)
)

HeartbeatHaptic::HeartbeatHaptic(GameObject* owner) : Script(owner)
{
}

void HeartbeatHaptic::Start()
{
    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatLub(1.0f, m_variant));
    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatDub(1.0f, m_variant));
}

void HeartbeatHaptic::fireLub(float t)
{
    const bool isHealth = (m_variant == HapticEffectDefinition::HeartbeatVariant::Health);

    const char* lubName = isHealth ? "HeartbeatLub_Health" : "HeartbeatLub_Separation";

    HeartbeatCycle cycle;
    if (isHealth)
        cycle = HeartbeatCycle::fromHealth(1.0f - t); // t is danger, so HP = 1 - t
    else
        cycle = HeartbeatCycle::fromSeparation(t);

    HapticAPI::playAtScale(lubName, t * m_hapticIntensity, 0);

    m_dubScale = t;
    m_dubTimer = cycle.interBeatSeconds;
    m_lubTimer = -1.0f;
}

void HeartbeatHaptic::Update()
{
    if (!m_active)
        return;

    const float dt = Time::getDeltaTime();
    const float t = m_currentT;

    if (m_dubTimer < 0.0f && m_lubTimer < 0.0f)
        fireLub(t);

    if (m_dubTimer >= 0.0f)
    {
        m_dubTimer -= dt;
        if (m_dubTimer < 0.0f)
        {
            const bool isHealth = (m_variant == HapticEffectDefinition::HeartbeatVariant::Health);
            const char* dubName = isHealth ? "HeartbeatDub_Health" : "HeartbeatDub_Separation";

            HapticAPI::playAtScale(dubName, m_dubScale * m_hapticIntensity, 0);

            if (m_dyingBeat)
            {
                m_dyingBeat = false;
                m_active = false;
                m_dubTimer = -1.0f;
                m_lubTimer = -1.0f;
                return;
            }

            HeartbeatCycle cycle;
            if (isHealth)
                cycle = HeartbeatCycle::fromHealth(1.0f - t);
            else
                cycle = HeartbeatCycle::fromSeparation(t);

            m_lubTimer = cycle.diastoleSeconds;
        }
    }

    if (m_lubTimer >= 0.0f)
    {
        m_lubTimer -= dt;
        if (m_lubTimer < 0.0f)
            fireLub(t);
    }
}

void HeartbeatHaptic::tick(float t)
{
    m_active = true;
    m_currentT = t;
}

void HeartbeatHaptic::stop()
{
    m_active = false;
    m_dubTimer = -1.0f;
    m_lubTimer = -1.0f;
    m_dubScale = 0.0f;
}

void HeartbeatHaptic::playDyingBeat(float t)
{
    m_dyingBeat = true;
    m_active = true;
    m_currentT = t;
    fireLub(t);
}

IMPLEMENT_SCRIPT(HeartbeatHaptic)