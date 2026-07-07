#pragma once

#include "ScriptAPI.h"

#include "Pickup.h"

class CooperativeSound;

class HealthPickup : public Pickup
{
    DECLARE_SCRIPT(HealthPickup)

public:
    explicit HealthPickup(GameObject* owner);

    void Start()  override;
    void Update() override;
    void OnTriggerEnter(GameObject* player) override;

    ScriptFieldList getExposedFields() const override;

    void setupDrop(float healAmount, const Vector3& landingPosition);

public:
    float   m_healAmount            = 10.0f;
    AssetRef<Prefab> m_collectParticlePrefab;
    float   m_spawnHeight           = 1.5f;   // fallback height when no custom spawn-from
    float   m_fallGravity           = 8.0f;
    Vector3 m_landingPosition       = Vector3::Zero;  // target floor position, set by spawner
    bool    m_hasCustomSpawnFrom    = false;

private:
    void idleAnimation();
    void fallAnimation();

private:
    bool    m_isFalling             = true;
    float   m_fallVelocity          = 0.0f;
    float   m_fallHVelocityX        = 0.0f;
    float   m_fallHVelocityZ        = 0.0f;

    Vector3 m_fallStartPosition     = Vector3::Zero;  // where the arc begins

    CooperativeSound* m_cooperativeSound = nullptr;

    float m_idleTimer               = 0.0f;
    float m_idleSpeed               = 0.2f;
    float m_horizontalAmplitude     = 0.1f;
    float m_verticalAmplitude       = 0.2f;
};
