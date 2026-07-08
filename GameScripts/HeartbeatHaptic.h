#pragma once

#include "ScriptAPI.h"
#include "HapticEffectDefinition.h"

class HeartbeatHaptic : public Script
{
    DECLARE_SCRIPT(HeartbeatHaptic)

public:
    explicit HeartbeatHaptic(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    float m_hapticIntensity = 1.0f;

    void tick(float t);

    void stop();

    void playDyingBeat(float t);

    HapticEffectDefinition::HeartbeatVariant m_variant = HapticEffectDefinition::HeartbeatVariant::Health;

private:
    void fireLub(float t);

    float m_dubTimer = -1.0f;
    float m_lubTimer = -1.0f;
    float m_dubScale = 0.0f;
    bool  m_dyingBeat = false;
    bool  m_active = false;
    float m_currentT = 0.0f;
};