#include "pch.h"
#include "Checkpoint1_Level1_Event.h"

Checkpoint1_Level1_Event::Checkpoint1_Level1_Event(GameObject* owner)
    : CheckpointEvent(owner)
{
}

void Checkpoint1_Level1_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_1_LEVEL_1; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint1_Level1_Event)