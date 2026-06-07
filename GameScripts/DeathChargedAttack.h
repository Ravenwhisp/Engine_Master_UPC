#pragma once

#include "DeathAbilityBase.h"

class DeathChargedAttack : public DeathAbilityBase
{
    DECLARE_SCRIPT(DeathChargedAttack)

public:
    explicit DeathChargedAttack(GameObject* owner);

    void Start()     override;
    void Update()    override;
    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

public:
    float m_chargedAttackDamage   = 40.0f;
    float m_arcRange              = 2.5f;
    float m_arcAngle              = 120.0f;
    float m_maxChargeTime         = 2.0f;
    float m_minChargeTime         = 0.5f;
    float m_attackLockDuration    = 0.4f;
    float m_finalHitLockDuration  = 0.8f;
    float m_chargedArcRange       = 3.5f;
    float m_chargedArcAngle       = 150.0f;

protected:
    void startAbility() override;

    bool canStartSpecificAbility() const override;

    void onAttackWindowUpdate()     override;
    void onAttackWindowFinished()   override;

private:
    void startCharging();
    void fireAttack();
    void dealDamageInArc(float damage, bool isChargedShot) const;
    void dealDamageInArc(float damage, float range, float angle, bool isChargedShot) const;
    void updateAimDirection();
    void snapFaceAimDirection();

    float   m_chargeTime             = 0.0f;
    bool    m_isCharging             = false;
    Vector3 m_aimDirection           = { 0.0f, 0.0f, 0.0f };

private:
    ScriptComponentRef<Transform> m_ChargedAttackUI;
	Transform* m_chargedAttackUITransform = nullptr;

    Transform* m_deathSlashUITransform = nullptr;
    UISlider* m_deathSlashUISlider = nullptr;

    void setupUI();
    void updateUI() override;
};
