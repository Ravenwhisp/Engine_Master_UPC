#include "pch.h"
#include "Checkpoint3_Level2_Event.h"

Checkpoint3_Level2_Event::Checkpoint3_Level2_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint3_Level2_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_3_LEVEL_2; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint3_Level2_Event)