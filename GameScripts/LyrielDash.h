#pragma once

#include "AbilityDash.h"

#define MAX_DASH_CHARGES 3

class LyrielDash : public AbilityDash
{
    DECLARE_SCRIPT(LyrielDash)

public:
    explicit LyrielDash(GameObject* owner);

    void Start() override;
    ScriptFieldList getExposedFields() const override;

    void recoverCharge();

protected:
    bool canDash() const override;
    void onDashStarted() override;
    void onDashUpdate(float dt) override;

public:
    float m_dashDurationLyriel = 0.15f;
    float m_dashDistanceLyriel = 3.0f;
    float m_dashCooldown = 0.5f;
    float m_chargeRechargeTime = 3.0f;

private:
    int m_charges = MAX_DASH_CHARGES;
    float m_chargeRecoveryTimer = 0.0f;
};