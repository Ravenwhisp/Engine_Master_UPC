#pragma once

#include "LyrielAbilityBase.h"

class LyrielArrowProjectile;
class LyrielParticles;
class LyrielUI;

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
    bool spawnArrowToDirection(const Vector3& direction);
    void faceTarget(GameObject* target);

    LyrielParticles* m_particles = nullptr;

protected:
    void onAttackWindowUpdate() override;
    void onAttackWindowFinished() override;

    float getCooldown() const override;

private:
    void beginAim();
    void updateAim();
    void updateAimUI();
    void releaseAimAndCast();

    bool canCast() const;

    Vector3 computeAimDirection() const;
    bool isAimStickValid(const Vector3& direction) const;

private:
    LyrielUI* m_lyrielUI = nullptr;

    bool m_isAiming = false;
    Vector3 m_currentAimDirection = Vector3::Zero;

    GameObject* m_attackFacingTarget = nullptr;
};