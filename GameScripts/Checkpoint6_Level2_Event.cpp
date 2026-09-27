#include "pch.h"
#include "Checkpoint6_Level2_Event.h"

Checkpoint6_Level2_Event::Checkpoint6_Level2_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint6_Level2_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_6_LEVEL_2; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint6_Level2_Event)