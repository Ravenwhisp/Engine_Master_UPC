#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint7_Level1_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint7_Level1_Event)

public:
    explicit Checkpoint7_Level1_Event(GameObject* owner);

    void Start() override;
};