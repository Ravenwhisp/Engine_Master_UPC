#include "pch.h"
#include "AmbientSoundLoop.h"

namespace
{
    constexpr const char* k_bank = "LevelCommon.bnk";
}

IMPLEMENT_SCRIPT(AmbientSoundLoop)

IMPLEMENT_SCRIPT_FIELDS(AmbientSoundLoop,
    SERIALIZED_STRING(m_playEvent, "Play Event"),
    SERIALIZED_STRING(m_stopEvent, "Stop Event"),
    SERIALIZED_BOOL(m_playOnStart, "Play On Start")
)

AmbientSoundLoop::AmbientSoundLoop(GameObject* owner)
    : Script(owner)
{
}

void AmbientSoundLoop::Start()
{
    m_source = AudioAPI::getSoundSourceComponent(getOwner());
    if (m_source == nullptr)
    {
        Debug::error("[AmbientSoundLoop] No SOUND_SOURCE component on '%s'.",
                     GameObjectAPI::getName(getOwner()));
        return;
    }

    if (m_playOnStart && !m_playEvent.empty())
    {
        m_playingID = AudioAPI::postEvent(m_source, k_bank, m_playEvent.c_str());
    }
}

void AmbientSoundLoop::stop()
{
    if (m_source == nullptr || m_stopEvent.empty())
    {
        return;
    }

    AudioAPI::postEvent(m_source, k_bank, m_stopEvent.c_str());
    m_playingID = 0;
}
