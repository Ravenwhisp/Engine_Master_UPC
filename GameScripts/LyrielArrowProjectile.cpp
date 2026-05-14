#include "pch.h"
#include "LyrielArrowProjectile.h"
#include "ArrowPool.h"
#include "Damageable.h"

LyrielArrowProjectile::LyrielArrowProjectile(GameObject* owner)
    : Script(owner)
{
}

void LyrielArrowProjectile::setPool(ArrowPool* pool)
{
    m_pool = pool;
}

bool LyrielArrowProjectile::isInUse() const
{
    return m_inUse;
}

void LyrielArrowProjectile::launch(const Vector3& start_position, const Vector3& direction, float speed, float lifetime, GameObject* target, float damage)
{
    m_inUse = true;
    m_direction = direction;
    m_direction.Normalize();
    m_speed = speed;
    m_lifeTimer = 0.0f;
    m_currentLifetime = lifetime;
    m_target = target;
    m_damage = damage;

    Transform* transform = GameObjectAPI::getTransform(getOwner());
    if (transform != nullptr)
    {
        TransformAPI::setGlobalPosition(transform, start_position);
        TransformAPI::lookAt(transform, start_position + m_direction);
    }

    GameObjectAPI::setActive(getOwner(), true);
}

void LyrielArrowProjectile::Update()
{
    if (!m_inUse)
    {
        return;
    }

    m_lifeTimer += Time::getDeltaTime();

    Transform* transform = GameObjectAPI::getTransform(getOwner());
    if (transform != nullptr)
    {
        TransformAPI::translateGlobal(transform, m_direction * m_speed * Time::getDeltaTime());
    }

    if (m_lifeTimer >= m_currentLifetime)
    {
        applyImpactDamage();
        returnToPool();
    }
}

void LyrielArrowProjectile::resetProjectile()
{
    m_inUse = false;
    m_direction = Vector3::Zero;
    m_speed = 0.0f;
    m_lifeTimer = 0.0f;
    m_currentLifetime = 0.0f;
    m_target = nullptr;
    m_damage = 0.0f;

    GameObjectAPI::setActive(getOwner(), false);
}

void LyrielArrowProjectile::returnToPool()
{
    if (m_pool == nullptr)
    {
        resetProjectile();
        return;
    }

    m_pool->releaseArrow(this);
}

void LyrielArrowProjectile::applyImpactDamage()
{
    if (m_target == nullptr)
    {
        return;
    }

    Script* script = GameObjectAPI::getScript(m_target, "Damageable");
    Damageable* damageable = dynamic_cast<Damageable*>(script);

    if (damageable != nullptr)
    {
        damageable->takeDamage(m_damage);
    }
}

IMPLEMENT_SCRIPT(LyrielArrowProjectile)