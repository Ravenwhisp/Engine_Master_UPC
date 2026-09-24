#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class EnemyAttackExecutor;
class AnimationComponent;
class ArthurUI;
class ArthurSound;
class CameraShake;

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

public:
    // -1 = left side
    // +1 = right side
    int m_sweepSide = 1;

private:
    ArthurBossController* m_arthurController = nullptr;
    EnemyAttackExecutor* m_attackExecutor = nullptr;
    AnimationComponent* m_animation = nullptr;
    ArthurUI* m_arthurUI = nullptr;
    ArthurSound* m_arthurSound = nullptr;
    CameraShake* m_cameraShake = nullptr;

    float m_stateTimer = 0.0f;

    bool m_hasAppliedHit = false;
};