#include "pch.h"
#include "ArthurSound.h"

namespace
{
    constexpr const char* k_hurt      = "Play_Arthur_Hurt";
    constexpr const char* k_footstep  = "Play_Arthur_Footsteps";

    constexpr const char* k_introRoar  = "Play_Arthur_Intro_Roar";
    constexpr const char* k_phase2Roar = "Play_Arthur_Phase2_Roar";
    constexpr const char* k_deathRoar  = "Play_Arthur_Death_Roar";

    constexpr const char* k_clawSwipe  = "Play_Arthur_Claw_Swipe";
    constexpr const char* k_clawImpact = "Play_Arthur_Claw_Impact";

    constexpr const char* k_sideSweep  = "Play_Arthur_Side_Sweep";
    constexpr const char* k_sideImpact = "Play_Arthur_Side_Impact";

    constexpr const char* k_preparingGrowl = "Play_Arthur_Preparing_Growl";
    constexpr const char* k_chargeSlam     = "Play_Arthur_Charge_Slam";
    constexpr const char* k_bodyImpact     = "Play_Arthur_Body_Impact";
    constexpr const char* k_gallopingStart = "Play_Arthur_Loop_Galloping";
    constexpr const char* k_gallopingStop  = "Stop_Arthur_Loop_Galloping";

    constexpr const char* k_hammerPreparing = "Play_Arthur_Hammer_Preparing";
    constexpr const char* k_hammerImpact    = "Play_Arthur_Hammer_Impact";
}

ArthurSound::ArthurSound(GameObject* owner)
    : EnemySound(owner)
{
}

const char* ArthurSound::evHurt()     const { return k_hurt; }
const char* ArthurSound::evFootstep() const { return k_footstep; }

void ArthurSound::playIntroRoar()  { postEvent(k_introRoar); }
void ArthurSound::playPhase2Roar() { postEvent(k_phase2Roar); }
void ArthurSound::playDeathRoar()  { postEvent(k_deathRoar); }

void ArthurSound::playClawSwipe()  { postEvent(k_clawSwipe); }
void ArthurSound::playClawImpact() { postEvent(k_clawImpact); }

void ArthurSound::playSideSweep()  { postEvent(k_sideSweep); }
void ArthurSound::playSideImpact() { postEvent(k_sideImpact); }

void ArthurSound::playPreparingGrowl() { postEvent(k_preparingGrowl); }
void ArthurSound::playChargeSlam()     { postEvent(k_chargeSlam); }
void ArthurSound::playBodyImpact()     { postEvent(k_bodyImpact); }

void ArthurSound::startGallopingLoop()
{
    if (m_gallopingLoopID != 0)
    {
        return;
    }
    m_gallopingLoopID = postEvent(k_gallopingStart);
}

void ArthurSound::stopGallopingLoop()
{
    if (m_gallopingLoopID == 0)
    {
        return;
    }
    postEvent(k_gallopingStop);
    m_gallopingLoopID = 0;
}

void ArthurSound::playHammerPreparing() { postEvent(k_hammerPreparing); }
void ArthurSound::playHammerImpact()    { postEvent(k_hammerImpact); }

void ArthurSound::stopAllLoops()
{
    EnemySound::stopAllLoops();
    stopGallopingLoop();
}

IMPLEMENT_SCRIPT(ArthurSound)
