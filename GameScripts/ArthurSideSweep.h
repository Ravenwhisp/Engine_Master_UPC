#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class ArthurAttackConfig;
class EnemyAttackExecutor;
class AnimationComponent;


class ArthurSideSweep : public StateMachineScript
{
    DECLARE_SCRIPT(ArthurSideSweep)

public:
    explicit ArthurSideSweep(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

    ScriptFieldList getExposedFields() const override;

private:
    void applyHit();
    void goToRecover();

	void setupUI();
	void updateUI();

public:
    // -1 = right side, +1 = left side.
    int m_sweepSide = 1;

private:
    ArthurBossController* m_arthurController = nullptr;
    ArthurAttackConfig* m_attackConfig = nullptr;
    EnemyAttackExecutor* m_attackExecutor = nullptr;
    AnimationComponent* m_animation = nullptr;

    float m_stateTimer = 0.0f;

    bool m_hasAppliedHit = false;
};