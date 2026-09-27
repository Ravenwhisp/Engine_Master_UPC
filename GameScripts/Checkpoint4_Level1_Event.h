#pragma once

#include "ScriptAPI.h"

#include "CheckpointEvent.h"

class Checkpoint4_Level1_Event : public CheckpointEvent
{
    DECLARE_SCRIPT(Checkpoint4_Level1_Event)
public:
    explicit Checkpoint4_Level1_Event(GameObject* owner);

    void Start() override;
};