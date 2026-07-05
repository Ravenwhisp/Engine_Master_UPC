#include "pch.h"
#include "MusicStateEvent.h"

#include "GameplayEventTrigger.h"
#include "MusicManager.h"
#include "MusicStates.h"

IMPLEMENT_SCRIPT_FIELDS(MusicStateEvent,
    SERIALIZED_ENUM_INT(m_stateOnEnter, "State On Enter", kMusicStateNames, kMusicStateCount),
    SERIALIZED_BOOL(m_changeOnExit, "Change State On Exit"),
    SERIALIZED_ENUM_INT(m_stateOnExit, "State On Exit", kMusicStateNames, kMusicStateCount)
)

MusicStateEvent::MusicStateEvent(GameObject* owner)
    : GameplayEventAction(owner)
{
}

void MusicStateEvent::executeEvent(GameplayEventTrigger* trigger)
{
    MusicManager* music = MusicManager::Get();
    if (music != nullptr && m_stateOnEnter >= 0 && m_stateOnEnter < kMusicStateCount)
    {
        music->SetMusicState(kMusicStateNames[m_stateOnEnter]);
    }
}

void MusicStateEvent::stopEvent(GameplayEventTrigger* trigger)
{
    if (!m_changeOnExit)
    {
        return;
    }

    MusicManager* music = MusicManager::Get();
    if (music != nullptr && m_stateOnExit >= 0 && m_stateOnExit < kMusicStateCount)
    {
        music->SetMusicState(kMusicStateNames[m_stateOnExit]);
    }
}

IMPLEMENT_SCRIPT(MusicStateEvent)
