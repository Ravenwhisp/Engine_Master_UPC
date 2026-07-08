#pragma once

#include "LyrielAbilityBase.h"
#include <vector>

class LyrielUI;

class LyrielChargedAttack : public LyrielAbilityBase
{
    DECLARE_SCRIPT(LyrielChargedAttack)

public:
    explicit LyrielChargedAttack(GameObject* owner);

    void Start() override;
    void Update() override;
    void drawGizmo() override;

protected:
    void startAbility() override;

    void onAttackWindowUpdate() override;
    void onAttackWindowFinished() override;

    float getCooldown() const override;

private:
    void beginCharge();
    void updateCharge();
    void releaseChargeAndShoot();

    bool canShoot() const;

    Vector3 computeAimDirection() const;
    float computeChargedDamage() const;
    float computeChargedRange() const;

    void collectEnemiesInLine(const Vector3& origin, const Vector3& forward, std::vector<GameObject*>& outTargets);
    bool applyChargedDamage(const std::vector<GameObject*>& targets, float damage);

    void spawnChargedArrow(const Vector3& origin, const Vector3& forward);
    void drawChargePreview(const Vector3& origin, const Vector3& forward) const;

    bool isAimStickValid(const Vector3& direction) const;

private:
    LyrielUI* m_lyrielUI = nullptr;

    bool m_isCharging = false;
    float m_chargeTimer = 0.0f;
    Vector3 m_currentAimDirection = Vector3::Zero;
    Vector3 m_attackFacingDirection = Vector3::Zero;
};