#include "pch.h"
#include "Checkpoint3_Level1_Event.h"

Checkpoint3_Level1_Event::Checkpoint3_Level1_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint3_Level1_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_3_LEVEL_1; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint3_Level1_Event)