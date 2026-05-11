#pragma once

#include "ScriptAPI.h"

class ArrowPool;

class LyrielArrowProjectile : public Script
{
    DECLARE_SCRIPT(LyrielArrowProjectile)

public:
    explicit LyrielArrowProjectile(GameObject* owner);

    void Update() override;

    void launch(const Vector3& start_position, const Vector3& direction, float speed, float lifetime, GameObject* target, float damage);
    void resetProjectile();
    void returnToPool();

    bool isInUse() const;
    void setPool(ArrowPool* pool);
    void setArrowOwnerTransform(Transform* transform);

private:
    void applyImpactDamage();

private:
    ArrowPool* m_pool = nullptr;
    Transform* m_arrowOwner = nullptr;

    bool m_inUse = false;
    Vector3 m_direction = Vector3::Zero;
    float m_speed = 0.0f;
    float m_currentLifetime = 0.0f;
    float m_lifeTimer = 0.0f;

    GameObject* m_target = nullptr;
    float m_damage = 0.0f;
};