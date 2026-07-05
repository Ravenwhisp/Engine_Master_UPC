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

    // One-shots.
    void playDefeated();    // both players downed → defeat feedback (before game over)
    void playHealthOrb();   // a player collected a health orb

    // Bound separation-damage loop (bank exposes an explicit Stop event).
    // Replaces the per-hit hurt grunt while the players are pulled apart.
    void startBoundDamageLoop();
    void stopBoundDamageLoop();

    // Revive loops (explicit Stop events). "Reviving" plays while a player is downed;
    // "Help Reviving" plays instead while the partner is in assist range.
    void startReviving();
    void stopReviving();
    void startHelpReviving();
    void stopHelpReviving();

    // Stops every cooperative loop (bound/reviving/help). One-shots are unaffected.
    void stopAllLoops();

private:
    uint32_t postEvent(const char* eventName);

    ComponentSoundSource* m_source = nullptr;

    // Loop playing-IDs double as "is active" flags so start/stop stay idempotent.
    uint32_t m_boundDamageLoopID  = 0;
    uint32_t m_revivingLoopID     = 0;
    uint32_t m_helpRevivingLoopID = 0;
};
