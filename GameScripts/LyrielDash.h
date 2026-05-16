#pragma once

#include "AbilityDash.h"

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

private:
    float m_chargeRechargeTime = 3.0f;
    int m_maxCharges = 3;
    int m_currentCharges = 0;
    float m_chargeRecoveryTimer = 0.0f;
};