#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint1_Level1_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint1_Level1_Event)

public:
    explicit Checkpoint1_Level1_Event(GameObject* owner);

    void Start() override;
};