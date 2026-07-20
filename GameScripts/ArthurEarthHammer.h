#pragma once

#include "ScriptAPI.h"
#include "StateMachineScript.h"

class ArthurBossController;
class ArthurAttackConfig;
class EnemyAttackExecutor;
class AnimationComponent;
class ArthurUI;
class ArthurSound;

class ArthurEarthHammer : public StateMachineScript
{
    DECLARE_SCRIPT(ArthurEarthHammer)

public:
    explicit ArthurEarthHammer(GameObject* owner);

    void OnStateEnter() override;
    void OnStateUpdate() override;
    void OnStateExit() override;

private:
    void applyImpact();
    void goToRecover();

private:
    ArthurBossController* m_arthurController = nullptr;
    AssetReference<ArthurAttackConfig> m_attackConfig;
    EnemyAttackExecutor* m_attackExecutor = nullptr;
    AnimationComponent* m_animation = nullptr;
    ArthurUI* m_arthurUI = nullptr;
    ArthurSound* m_arthurSound = nullptr;

    float m_stateTimer = 0.0f;

    bool m_hasAppliedImpact = false;
};