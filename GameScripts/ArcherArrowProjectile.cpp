#include "pch.h"
#include "ArcherArrowProjectile.h"

ArcherArrowProjectile::ArcherArrowProjectile(GameObject* owner) : ProjectileBase(owner) {}

void ArcherArrowProjectile::resetProjectile()
{
    ProjectileBase::resetProjectile();
    m_direction = Vector3::Zero;
    m_speed     = 0.0f;
    m_remaining = 0.0f;
    m_flying    = false;
    m_arrived   = false;
}

void ArcherArrowProjectile::launch(const Vector3& from, const Vector3& to, float speed)
{
    Vector3 delta = to - from;
    float dist = delta.Length();
    if (dist < 0.01f) return;

    m_direction = delta;
    m_direction.Normalize();
    m_speed     = speed;
    m_remaining = dist / speed;
    m_flying    = true;
    m_arrived   = false;

    GameObjectAPI::setActive(getOwner(), true);

    Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t)
    {
        TransformAPI::setGlobalPosition(t, from);
        TransformAPI::lookAt(t, from + m_direction);
    }
}

void ArcherArrowProjectile::Update()
{
    if (!m_flying) return;

    float dt = Time::getDeltaTime();
    m_remaining -= dt;

    Transform* t = GameObjectAPI::getTransform(getOwner());
    if (t)
        TransformAPI::translateGlobal(t, m_direction * m_speed * dt);

    if (m_remaining <= 0.0f)
    {
        m_flying  = false;
        m_arrived = true;
    }
}

IMPLEMENT_SCRIPT(ArcherArrowProjectile)
