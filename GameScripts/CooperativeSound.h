#pragma once

#include "ScriptAPI.h"

// Sits on the same GameObject as ReaperGauge / ShadowExecution (GameController).
// Plays the two cooperative-feedback events.
class CooperativeSound : public Script
{
    DECLARE_SCRIPT(CooperativeSound)

public:
    explicit CooperativeSound(GameObject* owner);

    void Start() override;

    // Triggered when both players press Triangle and the gauge is full → AoE execution.
    void playShadowExecution();

    // Triggered the frame the Reaper Gauge transitions from < full to full.
    void playReaperGaugeFull();

private:
    uint32_t postEvent(const char* eventName);

    ComponentSoundSource* m_source = nullptr;
};
