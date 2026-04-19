#pragma once

#include "ScriptAPI.h"

class Damageable;

class EnemyDeathHandler : public Script
{
    DECLARE_SCRIPT(EnemyDeathHandler)

public:
    explicit EnemyDeathHandler(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

private:
    void playDeathAnimation();
    void startDestroyCountdown(float delay);
    void destroyEnemyNow();
    void processDeath();

public:
    float m_destroyDelay = 2.0f;
    std::string m_deathStateName = "";

private:
    Damageable* m_damageable = nullptr;
    bool m_hasProcessedDeath = false;
    bool m_waitingToDestroy = false;
    float m_deathTimer = 0.0f;
};