#pragma once

#include "AbilityDash.h"

class DeathSound;

class DeathDash : public AbilityDash
{
    DECLARE_SCRIPT(DeathDash)

public:
    explicit DeathDash(GameObject* owner);

    void Start() override;
    ScriptFieldList getExposedFields() const override;

public:
    float m_dashDistance = 5.0f;
    float m_dashHitWidth = 2.0f;
    float m_dashDamage   = 20.0f;

protected:
    void onDashStarted() override;
    void onDashEnded() override;
    void onDashUpdate(float dt) override;

private:
    Vector3 m_dashStartPosition = Vector3::Zero;
    bool    m_dashDamageDealt = false;        // guard: damage fires only once per dash
    bool    m_dashImpactSoundPlayed = false;  // guard: impact sound fires once per dash, at first contact

    DeathSound* m_sound = nullptr;

    void applyDashDamage();
    bool isInsideDashRectangle(const Vector3& point) const;
    bool anyEnemyInsideDashRectangle() const;
};