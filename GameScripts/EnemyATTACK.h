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

private:
    GameObject* findPlayer() const;

public:
    float m_attackRadius = 2.5f;
    bool m_debugEnabled = true;
};