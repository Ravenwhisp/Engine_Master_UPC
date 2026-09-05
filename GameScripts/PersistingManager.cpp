#include "pch.h"
#include "PersistingManager.h"

#include "PersistingCheckpointState.h"
#include "PersistingPowerupState.h"

IMPLEMENT_SCRIPT_FIELDS(PersistingManager,
    SERIALIZED_INT(m_levelNumber, "Level number")
)

PersistingManager::PersistingManager(GameObject* owner)
    : Script(owner)
{
}

void PersistingManager::Start()
{
    if (PersistingCheckpointState::Get().m_lastCheckpointId != CheckpointId::NONE)
    {
        PersistingPowerupState::setUnlockedPowerupState(PersistingCheckpointState::Get().m_savedUnlockedPowerups);
    }

    PersistingCheckpointState::Get().m_deadEnemies.clear();
    PersistingCheckpointState::Get().m_brokenBreakables.clear();
    PersistingCheckpointState::Get().m_triggeredEvents.clear();
    PersistingCheckpointState::Get().m_solvedPuzzles.fill(false);
}

void PersistingManager::OnGameStop()
{
    PersistingPowerupState::reset();
    PersistingCheckpointState::Get().Reset();
}


IMPLEMENT_SCRIPT(PersistingManager)