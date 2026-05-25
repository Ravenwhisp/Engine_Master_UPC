#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class ArthurAttackConfig;
class ArthurAttackExecutor;

class ArthurChargingSlam : public StateMachineScript
{
    DECLARE_SCRIPT(ArthurChargingSlam)

public:
    explicit ArthurChargingSlam(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void lockTargetPosition();

    void startDash();
    void updateDash();

    void applyImpact();

    void tryApplyDashDamage(Transform* targetTransform, bool& hasDamagedTarget);

    void goToRecover();

private:
    ArthurBossController* m_arthurController = nullptr;
    ArthurAttackConfig* m_attackConfig = nullptr;
    ArthurAttackExecutor* m_attackExecutor = nullptr;

    float m_stateTimer = 0.0f;

    Vector3 m_startPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_lockedTargetPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_dashDirection = Vector3(0.0f, 0.0f, 0.0f);

    bool m_hasStartedDash = false;
    bool m_hasReachedDestination = false;
    bool m_hasAppliedImpact = false;

    bool m_hasDamagedFocusDuringDash = false;
    bool m_hasDamagedNonFocusDuringDash = false;
};