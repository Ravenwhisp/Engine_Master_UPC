#pragma once

#include "AbilityBase.h"

class DeathCharacter;

class DeathAbilityBase : public AbilityBase
{
public:
    explicit DeathAbilityBase(GameObject* owner);

    void Start() override;
    void Update() override;

protected:
    void beginAttackWindow(float lockDuration);
    void finishAttackWindow();
    void beginAttackPresentation();

    virtual void onAttackWindowUpdate()   {}
    virtual void onAttackWindowFinished() {}

protected:
    DeathCharacter* m_deathChar       = nullptr;
    float           m_attackStateTimer = 0.0f;
};
