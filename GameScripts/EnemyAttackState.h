#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class EnemyBaseController;
class EnemyBaseAttackConfig;
class AnimationComponent;
class EnemySound;
class PaladinVFX;

class EnemyAttackState : public StateMachineScript
{
    DECLARE_SCRIPT(EnemyAttackState)

public:
    explicit EnemyAttackState(GameObject* owner);

    FieldList getExposedFields() const override;

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void tryDamageTarget(Transform* targetTransform);
    void playBasicAttackEffect();

private:
    EnemyBaseController* m_controller = nullptr;
    AssetRef<EnemyBaseAttackConfig> m_attackConfig;
    AnimationComponent* m_animation = nullptr;
    PaladinVFX* m_paladinVFX = nullptr;
    EnemySound* m_enemySound = nullptr;

    Transform* m_committedTarget = nullptr;

    float m_stateTimer = 0.0f;
    bool m_hasAppliedDamage = false;
};