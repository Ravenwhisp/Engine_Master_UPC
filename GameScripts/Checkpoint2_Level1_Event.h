#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint2_Level1_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint2_Level1_Event)

public:
    explicit Checkpoint2_Level1_Event(GameObject* owner);

    void Start() override;
};