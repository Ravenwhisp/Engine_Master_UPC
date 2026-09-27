#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint6_Level2_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint6_Level2_Event)

public:
    explicit Checkpoint6_Level2_Event(GameObject* owner);

    void Start() override;
};