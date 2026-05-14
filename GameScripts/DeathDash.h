#pragma once

#include "AbilityDash.h"

class DeathDash : public AbilityDash
{
    DECLARE_SCRIPT(LyrielDash)

public:
    explicit DeathDash(GameObject* owner);

    void Start() override;
    ScriptFieldList getExposedFields() const override;

protected:

    void onDashStarted() override;
    void onDashEnded() override;
    void onDashUpdate(float dt) override;

public:
    float m_dashDurationLyriel = 0.15f;
    float m_dashDistanceLyriel = 3.0f;
    float m_dashCooldown = 4.0f;

    float m_dashHitWidth = 3.0f;

    float m_dashDamage = 20.0f;

private:
    Vector3 m_dashStartPosition = Vector3::Zero;
    bool    m_dashDamageDealt = false;   // guard: damage fires only once per dash

    void applyDashDamage();
    bool isInsideDashRectangle(const Vector3& point) const;
};