#pragma once

#include "Script.h"

class StateMachineScript : public Script
{
public:
    explicit StateMachineScript(GameObject* owner) : Script(owner) {}
    virtual ~StateMachineScript() = default;

    virtual void OnStateEnter() {}
    virtual void OnStateUpdate() {}
    virtual void OnStateExit() {}
};