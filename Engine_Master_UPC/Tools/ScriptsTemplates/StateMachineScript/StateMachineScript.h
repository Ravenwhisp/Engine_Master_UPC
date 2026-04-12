#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class $safeitemname$ : public StateMachineScript
{
    DECLARE_SCRIPT($safeitemname$)

public:
    explicit $safeitemname$(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

    ScriptFieldList getExposedFields() const override;
};