#include "pch.h"
#include "Checkpoint4_Level1_Event.h"

Checkpoint4_Level1_Event::Checkpoint4_Level1_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint4_Level1_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_4_LEVEL_1; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint4_Level1_Event)