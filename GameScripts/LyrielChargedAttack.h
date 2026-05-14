#pragma once

#include "LyrielAbilityBase.h"
#include <vector>

class LyrielChargedAttack : public LyrielAbilityBase
{
    DECLARE_SCRIPT(LyrielChargedAttack)

public:
    explicit LyrielChargedAttack(GameObject* owner);

    void Start() override;
    void Update() override;
    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

    ScriptComponentRef<Transform> m_ChargedAttackUI;

protected:
    void startAbility() override;

    void onAttackWindowUpdate() override;
    void onAttackWindowFinished() override;

private:
    void beginCharge();
    void updateCharge();
    void releaseChargeAndShoot();

    bool canStartCharge() const;
    bool canShoot() const;

    Vector3 computeAimDirection() const;
    float computeChargedDamage() const;
    float computeChargedRange() const;

    void collectEnemiesInLine(const Vector3& origin, const Vector3& forward, std::vector<GameObject*>& outTargets);
    void applyChargedDamage(const std::vector<GameObject*>& targets, float damage);

    void spawnChargedArrow(const Vector3& origin, const Vector3& forward);
    void drawChargePreview(const Vector3& origin, const Vector3& forward) const;

    bool isAimStickValid(const Vector3& direction) const;

private:
    bool m_isCharging = false;
    float m_chargeTimer = 0.0f;
    Vector3 m_currentAimDirection = Vector3::Zero;
    Vector3 m_attackFacingDirection = Vector3::Zero;

public:
    float m_minDamage = 5.0f;
    float m_maxDamage = 30.0f;
    float m_maxChargeTime = 2.0f;

    float m_minAttackRange = 4.0f;
    float m_maxAttackRange = 10.0f;
    float m_lineHalfWidth = 0.75f;

    float m_attackLockDuration = 0.3f;

    float m_arrowSpeed = 20.0f;
};