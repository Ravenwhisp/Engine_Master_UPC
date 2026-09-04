#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyBaseController;
class EnemyAttackExecutor;
class AnimationComponent;
class EnemySound;
class PaladinVFX;
class PaladinUI;

class EnemyAttackState : public StateMachineScript
{
    DECLARE_SCRIPT(EnemyAttackState)

public:
    explicit EnemyAttackState(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void tryDamageTarget(Transform* targetTransform);
    void applyPaladinAreaDamage();
    void lockPaladinAttackArea();
    void drawPaladinAttackTelegraph() const;
    void playBasicAttackEffect();

private:
    EnemyBaseController* m_controller = nullptr;
    EnemyAttackExecutor* m_attackExecutor = nullptr;
    AnimationComponent* m_animation = nullptr;
    PaladinVFX* m_paladinVFX = nullptr;
    EnemySound* m_enemySound = nullptr;
    PaladinUI* m_paladinUI = nullptr;

    Transform* m_committedTarget = nullptr;

    Vector3 m_lockedAttackOrigin = Vector3::Zero;
    Vector3 m_lockedAttackDirection = Vector3::Zero;
    Vector3 m_lockedTargetPosition = Vector3::Zero;

    float m_stateTimer = 0.0f;
    bool m_hasAppliedDamage = false;
    bool m_usePaladinAreaAttack = false;
};