#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class StateMachineTest : public StateMachineScript
{
    DECLARE_SCRIPT(StateMachineTest)

public:
    explicit StateMachineTest(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

    ScriptFieldList getExposedFields() const override;

public:
    float m_speed = 5.0f;
    bool m_debugEnabled = true;
};
