#include "pch.h"
#include "Checkpoint1_Level2_Event.h"

Checkpoint1_Level2_Event::Checkpoint1_Level2_Event(GameObject* owner)
    : CheckpointEvent(owner)
{
}

void Checkpoint1_Level2_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_1_LEVEL_2; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint1_Level2_Event)