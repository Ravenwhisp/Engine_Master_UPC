#pragma once

#include "ScriptAPI.h"
#include "ProjectileBase.h"

class LyrielArrowProjectile : public ProjectileBase
{
    DECLARE_SCRIPT(LyrielArrowProjectile)

public:
    explicit LyrielArrowProjectile(GameObject* owner);

    void Update() override;
    FieldList getExposedFields() const override;

    void launch(const Vector3& start_position, const Vector3& direction, float speed, float lifetime, GameObject* target, float damage);
    void resetProjectile() override;

private:
    void applyImpactDamage();
    void syncParticleTransform();

public:
    PrefabRef m_particlePrefab;

private:
    Vector3 m_direction = Vector3::Zero;
    float m_speed = 0.0f;
    float m_currentLifetime = 0.0f;
    float m_lifeTimer = 0.0f;

    GameObject* m_target = nullptr;
    float m_damage = 0.0f;

    GameObject* m_particleGO = nullptr;
};