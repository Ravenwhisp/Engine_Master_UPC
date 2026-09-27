#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint3_Level1_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint3_Level1_Event)

public:
    explicit Checkpoint3_Level1_Event(GameObject* owner);

    void Start() override;
};