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
    virtual void stopEvent(GameplayEventTrigger* trigger) {}
    
    void saveTriggeredEvent(GameplayEventTrigger* trigger);

protected:
    bool m_isPersistent = false;
};