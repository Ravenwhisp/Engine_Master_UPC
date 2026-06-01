#pragma once

#include "ScriptAPI.h"

#include <vector>

class DeathSound : public Script
{
    DECLARE_SCRIPT(DeathSound)

public:
    explicit DeathSound(GameObject* owner);

    void Start() override;
    void Update() override;

    // R1 Light Attack
    void playLightSwing();
    void playLightImpact();   // delayed internally to match animation contact frame

    // R2 Heavy Attack
    void playHeavySwing();
    void playHeavyImpact();   // delayed internally to match animation contact frame

    // R2 Hold (Charge) — loop only fires after a minimum hold time, so a quick
    // R2 tap (heavy swing without charge) does NOT trigger the charge sound.
    void startChargeLoop();
    void stopChargeLoop();
    void playChargeRelease(); // delayed internally to match animation contact frame

    // L1 Dash
    void playDashWhoosh();
    void playDashImpact();    // call ON FIRST CONTACT during the dash, not on dash end

    // L2 Taunt
    void playTauntShout();

    // Shadow Mark — delayed internally to match animation contact frame
    void playMarkApply();

    // Hover loop. Wwise has the event configured as an infinite loop.
    // start/stop are idempotent. Use setHoverActive(bool) as helper.
    void startHoverLoop();
    void stopHoverLoop();
    void setHoverActive(bool active);

    // Damage
    void playHurt();
    void playDown();

    // Targeting
    void playLockTarget();
    void playSwitchTarget();

    // Stops every active loop AND cancels pending delayed events. Call on death/despawn.
    void stopAllLoops();

private:
    uint32_t postEvent(const char* eventName);
    void     postEventDelayed(const char* eventName, float delay);

    ComponentSoundSource* m_source = nullptr;

    // Charge loop with minimum-hold arm: timer >= 0 = counting down to actual start;
    // < 0 = not armed.
    uint32_t m_chargeLoopID       = 0;
    float    m_chargeLoopArmTimer = -1.0f;

    // Hover loop
    uint32_t m_hoverLoopID = 0;

    struct PendingEvent
    {
        const char* eventName;
        float       timeRemaining;
    };
    std::vector<PendingEvent> m_pendingEvents;
};
