#include "pch.h"
#include "Checkpoint5_Level2_Event.h"

Checkpoint5_Level2_Event::Checkpoint5_Level2_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint5_Level2_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_5_LEVEL_2; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint5_Level2_Event)