#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint6_Level1_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint6_Level1_Event)

public:
    explicit Checkpoint6_Level1_Event(GameObject* owner);

    void Start() override;
};