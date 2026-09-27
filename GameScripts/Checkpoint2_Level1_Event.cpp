#include "pch.h"
#include "Checkpoint2_Level1_Event.h"

Checkpoint2_Level1_Event::Checkpoint2_Level1_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint2_Level1_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_2_LEVEL_1; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint2_Level1_Event)