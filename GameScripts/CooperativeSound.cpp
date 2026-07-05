#include "pch.h"
#include "CooperativeSound.h"

namespace
{
    constexpr const char* k_bank = "BoundByDeath.bnk";

    constexpr const char* k_shadowExecution  = "Play_Cooperative_Shadow_Execution";
    constexpr const char* k_reaperGaugeFull  = "Play_Cooperative_ReaperGauge_Full";

    constexpr const char* k_defeated         = "Play_Cooperative_Defeated";
    constexpr const char* k_healthOrb        = "Play_Cooperative_Health_Orb";

    constexpr const char* k_boundDamageStart = "Play_Cooperative_Bound_Damage";
    constexpr const char* k_boundDamageStop  = "Stop_Cooperative_Bound_Damage";
    constexpr const char* k_revivingStart    = "Play_Cooperative_Reviving";
    constexpr const char* k_revivingStop     = "Stop_Cooperative_Reviving";
    constexpr const char* k_helpRevivingStart = "Play_Cooperative_Help_Reviving";
    constexpr const char* k_helpRevivingStop  = "Stop_Cooperative_Help_Reviving";
}

CooperativeSound::CooperativeSound(GameObject* owner)
    : Script(owner)
{
}

void CooperativeSound::Start()
{
    m_source = AudioAPI::getSoundSourceComponent(getOwner());
    if (m_source == nullptr)
    {
        Debug::error("[CooperativeSound] No SOUND_SOURCE component on '%s'.",
                     GameObjectAPI::getName(getOwner()));
    }
}

uint32_t CooperativeSound::postEvent(const char* eventName)
{
    if (m_source == nullptr)
    {
        return 0;
    }
    return AudioAPI::postEvent(m_source, k_bank, eventName);
}

void CooperativeSound::playShadowExecution() { postEvent(k_shadowExecution); }
void CooperativeSound::playReaperGaugeFull() { postEvent(k_reaperGaugeFull); }

void CooperativeSound::playDefeated()  { postEvent(k_defeated); }
void CooperativeSound::playHealthOrb() { postEvent(k_healthOrb); }

void CooperativeSound::startBoundDamageLoop()
{
    if (m_boundDamageLoopID != 0)
    {
        return;
    }
    m_boundDamageLoopID = postEvent(k_boundDamageStart);
}

void CooperativeSound::stopBoundDamageLoop()
{
    if (m_boundDamageLoopID == 0)
    {
        return;
    }
    postEvent(k_boundDamageStop);
    m_boundDamageLoopID = 0;
}

void CooperativeSound::startReviving()
{
    if (m_revivingLoopID != 0)
    {
        return;
    }
    m_revivingLoopID = postEvent(k_revivingStart);
}

void CooperativeSound::stopReviving()
{
    if (m_revivingLoopID == 0)
    {
        return;
    }
    postEvent(k_revivingStop);
    m_revivingLoopID = 0;
}

void CooperativeSound::startHelpReviving()
{
    if (m_helpRevivingLoopID != 0)
    {
        return;
    }
    m_helpRevivingLoopID = postEvent(k_helpRevivingStart);
}

void CooperativeSound::stopHelpReviving()
{
    if (m_helpRevivingLoopID == 0)
    {
        return;
    }
    postEvent(k_helpRevivingStop);
    m_helpRevivingLoopID = 0;
}

void CooperativeSound::stopAllLoops()
{
    stopBoundDamageLoop();
    stopReviving();
    stopHelpReviving();
}

IMPLEMENT_SCRIPT(CooperativeSound)
