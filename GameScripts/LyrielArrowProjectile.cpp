#include "pch.h"
#include "LyrielArrowProjectile.h"
#include "EnemyDamageable.h"
#include "BreakableDamageable.h"
#include "LyrielCharacter.h"
#include "LyrielSound.h"
#include "ParticleLifecycle.h"
#include "LyrielParticles.h"

IMPLEMENT_SCRIPT_FIELDS(LyrielArrowProjectile,
    SERIALIZED_STRING(m_legacyParticlePath, "Particle Prefab Path"),
    SERIALIZED_ASSET_REF(m_particlePrefab, "Particle Prefab", AssetType::PREFAB)
)

LyrielArrowProjectile::LyrielArrowProjectile(GameObject* owner)
    : ProjectileBase(owner)
{
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

    syncParticleTransform();

    if (m_lifeTimer >= m_currentLifetime)
    {
        applyImpactDamage();
        returnToPool();
    }
}

void LyrielArrowProjectile::launch(const Vector3& startPosition, const Vector3& direction, float speed, float lifetime, GameObject* target, float damage)
{
    m_direction = direction;
    m_speed = speed;
    m_currentLifetime = lifetime;
    m_lifeTimer = 0.0f;

    m_target = target;
    m_damage = damage;

    Transform* transform = GameObjectAPI::getTransform(getOwner());

    if (transform != nullptr)
    {
        TransformAPI::setGlobalPosition(transform, startPosition);
    }

    m_inUse = true;

    GameObjectAPI::setActive(getOwner(), true);

    activateEmbeddedParticles();

    LyrielParticles* particles = getLyrielParticles();
    if (particles != nullptr)
    {
        particles->SetArrowTrailActive(transform);
    }
}

void LyrielArrowProjectile::resetProjectile()
{
    Transform* transform = GameObjectAPI::getTransform(getOwner());

    LyrielParticles* particles = getLyrielParticles();
    if (particles != nullptr)
    {
        particles->SetArrowTrailInactive(transform);
    }

    stopEmbeddedParticles();

    GameObjectAPI::setActive(getOwner(), false);

    m_direction = Vector3::Zero;
    m_speed = 0.0f;
    m_currentLifetime = 0.0f;
    m_lifeTimer = 0.0f;

    m_target = nullptr;
    m_damage = 0.0f;

    m_inUse = false;
}

void LyrielArrowProjectile::applyImpactDamage()
{
    if (m_target == nullptr)
    {
        return;
    }

    Transform* projectileOwner = getProjectileOwnerTransform();

    // Resolve LyrielSound on the shooter once for both impact + mark exploit feedback.
    LyrielSound* sound = nullptr;
    if (projectileOwner != nullptr)
    {
        GameObject* shooter = projectileOwner->getOwner();
        if (shooter != nullptr)
        {
            sound = GameObjectAPI::findScript<LyrielSound>(shooter);
        }
    }

    if (sound != nullptr)
    {
        sound->playArrowImpact();
    }

    EnemyDamageable* damageable = GameObjectAPI::findScript<EnemyDamageable>(m_target);

    if (damageable != nullptr)
    {
        EnemyHitContext ctx;
        ctx.damage = m_damage;
        ctx.attacker = projectileOwner;
        ctx.attackType = PlayerAttackType::LyrielArrow;

        damageable->takeDamage(ctx);
        if (damageable->lastHitExploitShadowMark() && projectileOwner != nullptr)
        {
            GameObject* shooter = projectileOwner->getOwner();

            if (shooter != nullptr)
            {
                LyrielCharacter* lyriel = GameObjectAPI::findScript<LyrielCharacter>(shooter);

                if (lyriel != nullptr)
                {
                    lyriel->onMarkExploited();
                }
            }
        }

        return;
    }

	BreakableDamageable* breakableDamageable = GameObjectAPI::findScript<BreakableDamageable>(m_target);

    if (breakableDamageable != nullptr)
    {
        breakableDamageable->takeDamage(m_damage);
    }
}

void LyrielArrowProjectile::activateEmbeddedParticles()
{
    ParticleLifecycle::restart(getOwner());
}

void LyrielArrowProjectile::stopEmbeddedParticles()
{
    ParticleLifecycle::stop(getOwner());
}

LyrielParticles* LyrielArrowProjectile::getLyrielParticles() const
{
    Transform* projectileOwner = getProjectileOwnerTransform();

    if (projectileOwner == nullptr)
    {
        return nullptr;
    }

    GameObject* lyriel = projectileOwner->getOwner();

    if (lyriel == nullptr)
    {
        return nullptr;
    }

    return GameObjectAPI::findScript<LyrielParticles>(lyriel);
}

void LyrielArrowProjectile::syncParticleTransform()
{
    if (m_particleGO == nullptr)
    {
        return;
    }

    Transform* arrowTransform = GameObjectAPI::getTransform(getOwner());
    Transform* particleTransform = GameObjectAPI::getTransform(m_particleGO);

    if (arrowTransform != nullptr && particleTransform != nullptr)
    {
        TransformAPI::setGlobalPosition(particleTransform, TransformAPI::getGlobalPosition(arrowTransform));
        TransformAPI::setGlobalRotationEuler(particleTransform, TransformAPI::getGlobalEulerDegrees(arrowTransform));
    }
}

IMPLEMENT_SCRIPT(LyrielArrowProjectile)
