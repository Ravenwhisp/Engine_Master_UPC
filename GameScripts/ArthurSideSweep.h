#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class ArthurAttackConfig;
class ArthurAttackExecutor;

class ArthurSideSweep : public StateMachineScript
{
    DECLARE_SCRIPT(ArthurSideSweep)

public:
    explicit ArthurSideSweep(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void applyHit();
    void goToRecover();

private:
    ArthurBossController* m_arthurController = nullptr;
    ArthurAttackConfig* m_attackConfig = nullptr;
    ArthurAttackExecutor* m_attackExecutor = nullptr;

    float m_stateTimer = 0.0f;

    bool m_hasAppliedHit = false;

    // -1 = right side, +1 = left side.
    int m_sweepSide = 1;
};