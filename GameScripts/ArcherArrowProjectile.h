#pragma once
#include "ProjectileBase.h"

class ArcherArrowProjectile : public ProjectileBase
{
    DECLARE_SCRIPT(ArcherArrowProjectile)
public:
    explicit ArcherArrowProjectile(GameObject* owner);
    void Update() override;

    void launch(const Vector3& from, const Vector3& to, float speed);
    bool hasArrived() const { return m_arrived; }

    void resetProjectile() override;

private:
    Vector3 m_direction = Vector3::Zero;
    float   m_speed     = 0.0f;
    float   m_remaining = 0.0f;
    bool    m_flying    = false;
    bool    m_arrived   = false;
};
