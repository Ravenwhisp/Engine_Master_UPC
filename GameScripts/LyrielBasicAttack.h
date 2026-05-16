#pragma once

#include "LyrielAbilityBase.h"

class LyrielArrowProjectile;

class LyrielBasicAttack : public LyrielAbilityBase
{
    DECLARE_SCRIPT(LyrielBasicAttack)

public:
    explicit LyrielBasicAttack(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

private:
    void startAbility() override;
    bool spawnArrowToTarget(GameObject* target);
    void faceTarget(GameObject* target);

protected:
    void onAttackWindowUpdate() override;
    void onAttackWindowFinished() override;

private:
    GameObject* m_attackFacingTarget = nullptr;

public:
    float m_attackDamage = 10.0f;
    float m_arrowSpeed = 18.0f;
    float m_attackLockDuration = 0.2f;
};