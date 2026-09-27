#include "pch.h"
#include "Checkpoint5_Level1_Event.h"

Checkpoint5_Level1_Event::Checkpoint5_Level1_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint5_Level1_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_5_LEVEL_1; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint5_Level1_Event)