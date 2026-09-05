#include "pch.h"
#include "CameraShake.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(CameraShake,
    SERIALIZED_FLOAT(m_impactIntensity, "Impact Intensity", 0.0f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_impactDuration, "Impact Duration", 0.0f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_roarIntensity, "Roar Intensity", 0.0f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_roarDuration, "Roar Duration", 0.0f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_lightIntensity, "Light Intensity", 0.0f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_lightDuration, "Light Duration", 0.0f, 5.0f, 0.01f),
    SERIALIZED_FLOAT(m_frequency, "Frequency", 0.0f, 60.0f, 0.1f),
    SERIALIZED_FLOAT(m_verticalScale, "Vertical Scale", 0.0f, 1.0f, 0.01f)
)

CameraShake::CameraShake(GameObject* owner)
    : Script(owner)
{
}

void CameraShake::shakeImpact()
{
    shake(m_impactIntensity, m_impactDuration);
}

void CameraShake::shakeRoar()
{
    shake(m_roarIntensity, m_roarDuration);
}

void CameraShake::shakeLight()
{
    shake(m_lightIntensity, m_lightDuration);
}

void CameraShake::shake(float intensity, float duration)
{
    if (intensity <= 0.0f || duration <= 0.0f)
    {
        return;
    }

    // Don't let a weaker shake cut a stronger one short.
    const float currentStrength = m_active ? m_amplitude * envelope() : 0.0f;

    if (intensity < currentStrength)
    {
        return;
    }

    m_amplitude = intensity;
    m_duration = duration;
    m_timer = 0.0f;
    m_active = true;
}

void CameraShake::Update()
{
    if (!m_active)
    {
        return;
    }

    m_timer += Time::getDeltaTime();

    if (m_timer >= m_duration)
    {
        m_active = false;
        m_currentOffset = Vector3(0.0f, 0.0f, 0.0f);
        return;
    }

    const float magnitude = m_amplitude * envelope();

    const float phase = m_timer * m_frequency * MathAPI::TWO_PI;

    const float noiseX = sinf(phase);
    const float noiseY = sinf(phase * 1.3f + 2.1f);
    const float noiseZ = sinf(phase * 0.9f + 4.2f);

    m_currentOffset = Vector3(
        noiseX * magnitude,
        noiseY * magnitude * m_verticalScale,
        noiseZ * magnitude
    );
}

float CameraShake::envelope() const
{
    if (m_duration <= 0.0001f)
    {
        return 0.0f;
    }

    float p = m_timer / m_duration;

    if (p < 0.0f)
    {
        p = 0.0f;
    }

    if (p > 1.0f)
    {
        p = 1.0f;
    }

    return (1.0f - p) * (1.0f - p);
}

IMPLEMENT_SCRIPT(CameraShake)
