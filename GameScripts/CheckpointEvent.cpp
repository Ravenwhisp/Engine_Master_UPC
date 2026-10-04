#include "pch.h"
#include "CheckpointEvent.h"

#include "PersistingPowerupState.h"

IMPLEMENT_SCRIPT_FIELDS(CheckpointEvent,
	SERIALIZED_COMPONENT_REF(m_lyrielRespawn, "Lyriel respawn transform", ComponentType::TRANSFORM),
	SERIALIZED_COMPONENT_REF(m_deathRespawn, "Death respawn transform", ComponentType::TRANSFORM)
)

CheckpointEvent::CheckpointEvent(GameObject* owner)
    : GameplayEventAction(owner)
{
}

void CheckpointEvent::Start()
{
	m_PersistingCheckpointState = &PersistingCheckpointState::Get();

	if (!m_PersistingCheckpointState)
	{
		Debug::warn("CheckpointEvent: PersistingCheckpointState singleton not found.");
	}

	m_lyrielRespawnTransform = m_lyrielRespawn.getReferencedComponent();
	if (!m_lyrielRespawnTransform)
	{
		Debug::warn("CheckpointEvent: No LyrielRespawn transform referenced.");
	}

	m_deathRespawnTransform = m_deathRespawn.getReferencedComponent();
	if (!m_deathRespawnTransform)
	{
		Debug::warn("CheckpointEvent: No DeathRespawn transform referenced.");
	}
}

void CheckpointEvent::Update()
{
}

void CheckpointEvent::executeEvent(GameplayEventTrigger* trigger)
{
	if(m_PersistingCheckpointState)
	{
		if(m_PersistingCheckpointState->m_lastCheckpointId >= m_checkpointId)
		{
			Debug::log("CheckpointEvent: Checkpoint %d already saved, skipping.", static_cast<int>(m_checkpointId));
			return;
		}
		bool* currentPowerups = PersistingPowerupState::getUnlockedPowerupState();

		if(m_lyrielRespawnTransform)
		{
			m_PersistingCheckpointState->m_savedLyrielRespawn = TransformAPI::getGlobalPosition(m_lyrielRespawnTransform);
		}
		if (m_deathRespawnTransform)
		{
			m_PersistingCheckpointState->m_savedDeathRespawn = TransformAPI::getGlobalPosition(m_deathRespawnTransform);
		}

		std::copy(currentPowerups,
			currentPowerups + static_cast<int>(PowerupId::Count),
			m_PersistingCheckpointState->m_savedUnlockedPowerups);

		m_PersistingCheckpointState->SetCheckpoint(m_checkpointId);
		Debug::log("CheckpointEvent: Checkpoint %d saved.", static_cast<int>(m_checkpointId));
	}
}

IMPLEMENT_SCRIPT(CheckpointEvent)
