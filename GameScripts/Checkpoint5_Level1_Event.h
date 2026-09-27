#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint5_Level1_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint5_Level1_Event)

public:
    explicit Checkpoint5_Level1_Event(GameObject* owner);

    void Start() override;
};