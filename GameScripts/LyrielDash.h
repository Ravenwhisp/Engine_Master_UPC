#pragma once

#include "AbilityDash.h"

class LyrielSound;
class LyrielUI;
class LyrielCharacter;
class LyrielConfig;
class LyrielParticles;

class LyrielDash : public AbilityDash
{
    DECLARE_SCRIPT(LyrielDash)

public:
    explicit LyrielDash(GameObject* owner);

    FieldList getExposedFields() const override;

    void Start() override;

    void recoverCharge();

protected:
    float getCooldown() const override;
    float getDashDuration() const override;
    float getDashDistance() const override;

    bool canDash() const override;
    void onDashStarted() override;
    void onDashUpdate(float dt) override;
    void onDashEnded() override;
    bool validateDashTarget() override;
    void drawGizmo() override;

private:
    LyrielCharacter* m_lyrielCharacter = nullptr;
    AssetReference<LyrielConfig> m_config;
    LyrielUI* m_lyrielUI = nullptr;

    int m_currentCharges = 0;
    float m_chargeRecoveryTimer = 0.0f;

private:
    // For debugging only
    Vector3 m_debugDashStart = Vector3::Zero;
    Vector3 m_debugDashCandidateEnd = Vector3::Zero;
    Vector3 m_debugDashSampleEnd = Vector3::Zero;
    bool m_debugLastDashValid = false;

    LyrielSound* m_sound = nullptr;
    LyrielParticles* m_particles = nullptr;
};