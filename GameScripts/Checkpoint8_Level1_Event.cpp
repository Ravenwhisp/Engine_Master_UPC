#include "pch.h"
#include "Checkpoint8_Level1_Event.h"

Checkpoint8_Level1_Event::Checkpoint8_Level1_Event(GameObject* owner)
	: CheckpointEvent(owner)
{
}

void Checkpoint8_Level1_Event::Start()
{
	CheckpointEvent::Start();

	m_checkpointId = CheckpointId::CHECKPOINT_8_LEVEL_1; // Set the checkpoint ID for this specific event
}

IMPLEMENT_SCRIPT(Checkpoint8_Level1_Event)