#include "pch.h"
#include "CooperativeSound.h"

namespace
{
    constexpr const char* k_bank = "BoundByDeath.bnk";

    constexpr const char* k_shadowExecution  = "Play_Cooperative_Shadow_Execution";
    constexpr const char* k_reaperGaugeFull  = "Play_Cooperative_ReaperGauge_Full";
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

IMPLEMENT_SCRIPT(CooperativeSound)
