#pragma once

#include "ScriptAPI.h"

class Transform;
class Damageable;
class HeartbeatHaptic;
class CooperativeSound;
class BoundConfig;

class Bound : public Script
{
    DECLARE_SCRIPT(Bound)

public:
    explicit Bound(GameObject* owner);

    void Start() override;
    void Update() override;

    void drawGizmo() override;

    FieldList getExposedFields() const override;

public:
    ComponentRef<Transform> m_firstTarget;
    ComponentRef<Transform> m_secondTarget;

    ComponentRef<Transform> m_BoundUI;

    AssetRef<BoundConfig> m_config;

    Damageable* m_firstDamageable = nullptr;
    Damageable* m_secondDamageable = nullptr;

    float m_minDistance = 70.0f;
    float m_distanceDamage = 80.0f;
    float m_distanceInstaKill = 100.0f;

    float baseDamage = 20.0f;
    float maxDamage = 40.0f;

    float m_radiusThreshold = 2.0f;

    Vector3 m_center = Vector3(0.0f, 0.0f, 0.0f);
    float   m_currentRadius = 0.0f;

    float m_previousDistance = 0.0f;

    float m_separationHapticHpGate = 0.5f;

private:
    HeartbeatHaptic* m_haptic = nullptr;
    CooperativeSound* m_coopSound = nullptr;
};