#include "pch.h"
#include "PaladinSound.h"

namespace
{
    constexpr const char* k_basicSwing    = "Play_Paladin_Basic_Swing";
    constexpr const char* k_basicImpact   = "Play_Paladin_Basic_Impact";
    constexpr const char* k_hurt          = "Play_Paladin_Hurt";
    constexpr const char* k_stun          = "Play_Paladin_Stun";
    constexpr const char* k_death         = "Play_Paladin_Death";
    constexpr const char* k_footstep      = "Play_PaladinFootsteps";

    constexpr const char* k_chargeStart   = "Play_Paladin_Charge_Start";
    constexpr const char* k_chargeLoop     = "Play_Paladin_Charge_Loop";
    constexpr const char* k_chargeLoopStop = "Stop_Paladin_Charge_Loop";
    constexpr const char* k_chargeImpact  = "Play_Paladin_Charge_Impact";
}

PaladinSound::PaladinSound(GameObject* owner)
    : EnemySound(owner)
{
}

const char* PaladinSound::evBasicTelegraph() const { return k_basicSwing; }
const char* PaladinSound::evBasicImpact()    const { return k_basicImpact; }
const char* PaladinSound::evHurt()           const { return k_hurt; }
const char* PaladinSound::evStun()           const { return k_stun; }
const char* PaladinSound::evDeath()          const { return k_death; }
const char* PaladinSound::evFootstep()       const { return k_footstep; }

void PaladinSound::playChargeStart() { postEvent(k_chargeStart); }

void PaladinSound::startChargeLoop()
{
    if (m_chargeLoopID != 0)
    {
        return;
    }
    m_chargeLoopID = postEvent(k_chargeLoop);
}

void PaladinSound::stopChargeLoop()
{
    if (m_chargeLoopID == 0)
    {
        return;
    }
    postEvent(k_chargeLoopStop);
    m_chargeLoopID = 0;
}

void PaladinSound::playChargeImpact() { postEvent(k_chargeImpact); }

void PaladinSound::stopAllLoops()
{
    EnemySound::stopAllLoops();
    stopChargeLoop();
}

IMPLEMENT_SCRIPT(PaladinSound)
