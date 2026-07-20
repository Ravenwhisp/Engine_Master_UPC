#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class ArthurAttackConfig;
class EnemyAttackExecutor;
class AnimationComponent;
class ArthurUI;
class ArthurSound;

class ArthurHeavySwipe : public StateMachineScript
{
    DECLARE_SCRIPT(ArthurHeavySwipe)

public:
    explicit ArthurHeavySwipe(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void tryApplyHit(int hitIndex);
    void goToRecover();

private:
    ArthurBossController* m_arthurController = nullptr;
    AssetReference<ArthurAttackConfig> m_attackConfig;
    EnemyAttackExecutor* m_attackExecutor = nullptr;
    AnimationComponent* m_animation = nullptr;
    ArthurUI* m_arthurUI = nullptr;
    ArthurSound* m_arthurSound = nullptr;

    float m_stateTimer = 0.0f;

    bool m_hit1Applied = false;
    bool m_hit2Applied = false;
    bool m_hit3Applied = false;
    bool m_hit4Applied = false;

    float m_previousAnimationSpeed = 1.0f;

    float m_phase1AnimationSpeed = 2.0f;
    float m_phase2AnimationSpeed = 2.7f;
};