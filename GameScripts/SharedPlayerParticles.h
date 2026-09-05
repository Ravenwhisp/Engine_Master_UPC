#pragma once

#include "ScriptAPI.h"

class SharedPlayerParticles final : public Script
{
    DECLARE_SCRIPT(SharedPlayerParticles)

public:
    explicit SharedPlayerParticles(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

    void playDamageParticle();

private:
    void ensureDamageParticle();
    PrefabRef m_damageParticlePrefab;

    float m_damageParticleLifetime = 0.75f;
    float m_damageParticleCooldown = 0.25f;

    GameObject* m_activeDamageParticle = nullptr;

    Transform* m_ownerTransform = nullptr;
    Transform* m_activeDamageParticleTransform = nullptr;

    float m_damageParticleTimer = 0.0f;
    float m_damageParticleCooldownTimer = 0.0f;
};