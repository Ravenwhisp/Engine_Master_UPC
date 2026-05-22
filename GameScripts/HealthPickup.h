#pragma once

#include "ScriptAPI.h"

class HealthPickup : public Script
{
    DECLARE_SCRIPT(HealthPickup)

public:
    explicit HealthPickup(GameObject* owner);

    void Start()  override;
    void Update() override;
    void OnTriggerEnter(GameObject* player) override;

    ScriptFieldList getExposedFields() const override;

private:
    void idleAnimation();
    void fallAnimation();

public:
    float   m_healAmount            = 10.0f;
    float   m_spawnHeight           = 1.5f;   // fallback height when no custom spawn-from
    float   m_fallGravity           = 8.0f;
    Vector3 m_landingPosition      = Vector3::Zero;  // target floor position, set by spawner
    bool    m_hasCustomSpawnFrom   = false;

private:
    bool    m_collected             = false;
    bool    m_isFalling             = true;
    float   m_fallVelocity          = 0.0f;
    float   m_fallHVelocityX        = 0.0f;
    float   m_fallHVelocityZ        = 0.0f;

    Vector3 m_startPosition         = Vector3::Zero;  // landing target
    Vector3 m_fallStartPosition     = Vector3::Zero;  // where the arc begins

    float m_idleTimer           = 0.0f;
    float m_idleSpeed           = 0.2f;
    float m_horizontalAmplitude = 0.1f;
    float m_verticalAmplitude   = 0.2f;
};
