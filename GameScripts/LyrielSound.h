#pragma once

#include "ScriptAPI.h"

class LyrielSound : public Script
{
    DECLARE_SCRIPT(LyrielSound)

public:
    explicit LyrielSound(GameObject* owner);

    void Start() override;
    void Update() override;

    // R1 Basic Attack
    void playBowRelease();
    void playArrowImpact();   // posted by the projectile the frame it lands

    // R2 Charged Attack — tense is a loop event (no Stop event in bank, stopped by playingID).
    // Armed with a minimum hold time so a quick R2 tap plays no tense at all.
    void startChargedTenseLoop();
    void stopChargedTenseLoop();
    void playChargedRelease();
    void playChargedImpact();

    // L1 Dash
    void playDashWhoosh();

    // L2 Arrow Volley
    void playVolleyRelease();

    // Shadow Mark — Lyriel exploits (Phase 3 burst). Once per cast/hit.
    void playMarkExploit();

    // Footsteps — Wwise Switch Container handles walk/run × surface; we only post.
    // Surface defaults to whatever the Wwise project defines as default (set to Stone).
    void setFootstepsActive(bool active);

    // Damage
    void playHurt();
    void playDown();

    // Targeting
    void playLockTarget();
    void playSwitchTarget();

    // Stops every active loop AND footstep emission. Call on death/despawn.
    void stopAllLoops();

private:
    uint32_t postEvent(const char* eventName);

    ComponentSoundSource* m_source = nullptr;

    // Charged tense loop with minimum-hold arm. Stopped via stopEvent(playingID)
    // because the bank exposes no Stop event for this loop.
    uint32_t m_chargedTenseLoopID = 0;
    float    m_chargedTenseArmTimer = -1.0f;

    // Footstep ticking
    bool  m_footstepsActive = false;
    float m_footstepTimer   = 0.0f;
};
