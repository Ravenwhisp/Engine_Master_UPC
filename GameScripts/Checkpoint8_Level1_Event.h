#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint8_Level1_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint8_Level1_Event)

public:
    explicit Checkpoint8_Level1_Event(GameObject* owner);

    void Start() override;
};