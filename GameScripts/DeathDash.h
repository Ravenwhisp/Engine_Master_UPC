#pragma once

#include "AbilityDash.h"

class DeathSound;
class DeathCharacter;
class DeathConfig;
class DeathParticles;

class DeathDash : public AbilityDash
{
    DECLARE_SCRIPT(DeathDash)

public:
    explicit DeathDash(GameObject* owner);

    FieldList getExposedFields() const override;

    void Start() override;

protected:
    float getCooldown() const override;
    float getDashDuration() const override;
    float getDashDistance() const override;

    void onDashStarted() override;
    void onDashEnded() override;
    void onDashUpdate(float dt) override;

    void applyDashDamage();
    bool isInsideDashRectangle(const Vector3& point) const;
    bool anyEnemyInsideDashRectangle() const;

private:
    DeathCharacter* m_deathCharacter = nullptr;
    AssetRef<DeathConfig> m_config;
    DeathSound* m_sound = nullptr;
    DeathParticles* m_particles = nullptr;

    Vector3 m_dashStartPosition = Vector3::Zero;
    bool    m_dashDamageDealt = false;        // guard: damage fires only once per dash
    bool    m_dashImpactSoundPlayed = false;  // guard: impact sound fires once per dash, at first contact
};