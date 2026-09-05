#pragma once

#include "ScriptAPI.h"

class Transform;
class Transform2D;
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

    ComponentRef<Transform> m_boundUI;
    ComponentRef<Transform2D> m_boundaryWarningUI;

    Damageable* m_firstDamageable = nullptr;
    Damageable* m_secondDamageable = nullptr;

    float m_minDistance = 70.0f;
    float m_showBoundDistance = 50.0f;

    float baseDamage = 20.0f;

    float m_radiusThreshold = 2.0f;

    Vector3 m_center = Vector3(0.0f, 0.0f, 0.0f);
    float   m_currentRadius = 0.0f;

    float m_separationHapticHpGate = 0.5f;

    AssetReference<BoundConfig> m_config;

private:
    HeartbeatHaptic* m_haptic = nullptr;
    CooperativeSound* m_coopSound = nullptr;
};