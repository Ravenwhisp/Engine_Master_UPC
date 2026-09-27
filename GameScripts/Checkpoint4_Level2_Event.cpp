#include "pch.h"
#include "Checkpoint4_Level2_Event.h"

Checkpoint4_Level2_Event::Checkpoint4_Level2_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint4_Level2_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_4_LEVEL_2; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint4_Level2_Event)