#include "pch.h"
#include "Checkpoint7_Level1_Event.h"

Checkpoint7_Level1_Event::Checkpoint7_Level1_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint7_Level1_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_7_LEVEL_1; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint7_Level1_Event)