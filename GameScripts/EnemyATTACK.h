#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyATTACK : public StateMachineScript
{
    DECLARE_SCRIPT(EnemyATTACK)

public:
    explicit EnemyATTACK(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

    ScriptFieldList getExposedFields() const override;

public:
    float m_speed = 5.0f;
    bool m_debugEnabled = true;
};