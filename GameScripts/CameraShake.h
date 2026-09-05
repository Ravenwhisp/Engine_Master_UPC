#pragma once

#include "ScriptAPI.h"

// Lives on the camera GameObject next to CameraFollow. Holds all the shake logic and
// tunables; gameplay just calls a named preset (shakeImpact / shakeRoar) as a one-liner.
// CameraFollow queries getCurrentOffset() and adds it on top of the followed position,
// so the shake never fights the follow and always settles back to zero.
class CameraShake : public Script
{
    DECLARE_SCRIPT(CameraShake)

public:
    explicit CameraShake(GameObject* owner);

    void Update() override;

    FieldList getExposedFields() const override;

    void shakeImpact();
    void shakeRoar();
    void shakeLight();

    void shake(float intensity, float duration);

    Vector3 getCurrentOffset() const { return m_currentOffset; }

public:
    float m_impactIntensity = 0.35f;
    float m_impactDuration = 0.45f;

    float m_roarIntensity = 0.20f;
    float m_roarDuration = 0.80f;

    float m_lightIntensity = 0.12f;
    float m_lightDuration = 0.20f;

    float m_frequency = 22.0f;
    float m_verticalScale = 0.5f;

private:
    float envelope() const;

    float m_amplitude = 0.0f;
    float m_duration = 0.0f;
    float m_timer = 0.0f;
    bool m_active = false;

    Vector3 m_currentOffset = Vector3(0.0f, 0.0f, 0.0f);
};
