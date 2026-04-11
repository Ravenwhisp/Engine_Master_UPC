#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyCHASE : public StateMachineScript
{
    DECLARE_SCRIPT(EnemyCHASE)

public:
    explicit EnemyCHASE(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

    ScriptFieldList getExposedFields() const override;

private:
    GameObject* findPlayer() const;

public:
    float m_speed = 3.5f;
    float m_attackRadius = 2.25f;
    float m_loseRadius = 12.0f;
    bool m_debugEnabled = true;
};