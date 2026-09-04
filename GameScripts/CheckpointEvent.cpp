#include "pch.h"
#include "CheckpointEvent.h"

#include "ReaperGauge.h"
#include "Bound.h"
#include "Damageable.h"
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
	auto managers = SceneAPI::findAllGameObjectsWithScript<ReaperGauge>();
	GameObject* manager = nullptr;
	for (GameObject* obj : managers)
	{
		m_reaperGauge = GameObjectAPI::findScript<ReaperGauge>(obj);
		if (m_reaperGauge)
		{
			manager = obj;
			break;
		}
	}
	if (manager)
	{
		Bound* boundScript = GameObjectAPI::findScript<Bound>(manager);
		m_lyrielDamageable = boundScript ? boundScript->m_firstDamageable : nullptr;
		m_deathDamageable = boundScript ? boundScript->m_secondDamageable : nullptr;
	}

	m_PersistingCheckpointState = &PersistingCheckpointState::Get();

	if (!m_PersistingCheckpointState)
	{
		Debug::warn("CheckpointEvent: PersistingCheckpointState singleton not found.");
	}

	if (!m_reaperGauge)
	{
		Debug::warn("CheckpointEvent: ReaperGauge script not found in scene.");
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

		m_PersistingCheckpointState->m_savedLyrielHealth = m_lyrielDamageable ? m_lyrielDamageable->getCurrentHp() : 0.0f;
		m_PersistingCheckpointState->m_savedDeathHealth = m_deathDamageable ? m_deathDamageable->getCurrentHp() : 0.0f;
		m_PersistingCheckpointState->m_savedReaperGaugeAmount = m_reaperGauge ? m_reaperGauge->getGauge() : 0.0f;

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