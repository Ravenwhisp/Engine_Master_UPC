#pragma once

#include "ScriptAPI.h"

class GameplayEventTrigger;

class GameplayEventAction : public Script
{
public:
    explicit GameplayEventAction(GameObject* owner)
        : Script(owner)
    {
    }

    virtual ~GameplayEventAction() = default;

    virtual void executeEvent(GameplayEventTrigger* trigger) = 0;
};