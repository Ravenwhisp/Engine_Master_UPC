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

private:
    void startAbility() override;
    bool spawnArrowToTarget(GameObject* target);
    void faceTarget(GameObject* target);

protected:
    void onAttackWindowUpdate() override;
    void onAttackWindowFinished() override;

    float getCooldown() const override;

private:
    GameObject* m_attackFacingTarget = nullptr;
};