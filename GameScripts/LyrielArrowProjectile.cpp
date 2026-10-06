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
    SERIALIZED_ASSET_REF(m_particlePrefab, "Particle Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_visualBasicPrefab, "Visual Basic Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_visualChargedPrefab, "Visual Charged Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_visualVolleyPrefab, "Visual Volley Prefab", AssetType::PREFAB)
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

void LyrielArrowProjectile::launch(const Vector3& startPosition, const Vector3& direction, float speed, float lifetime, GameObject* target, float damage, VisualModel visual)
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
        TransformAPI::lookAt(transform, startPosition + m_direction);
    }

    // Orient projectile to face the travel direction. Use lookAt then rotate 180 degrees
    // if models are authored facing the opposite direction.
    if (transform != nullptr)
    {
        // Use a target point in the direction the arrow will travel
        TransformAPI::lookAt(transform, startPosition + direction);

        // Many arrow visuals are modeled pointing towards -Z; flip 180 degrees around Y so
        // the visual faces along the movement direction.
        Vector3 euler = TransformAPI::getGlobalEulerDegrees(transform);
        euler.y += 180.0f;
        // Normalize angle into [-180,180] range is optional but harmless
        if (euler.y > 180.0f) euler.y -= 360.0f;
        TransformAPI::setGlobalRotationEuler(transform, euler);
    }

    m_inUse = true;

    GameObjectAPI::setActive(getOwner(), true);

	//Instace model based on the visual model selected
    PrefabRef* chosenPrefab = nullptr;
    switch (visual)
    {
    case VisualModel::Basic:
        chosenPrefab = &m_visualBasicPrefab;
        break;
    case VisualModel::Charged:
        chosenPrefab = &m_visualChargedPrefab;
        break;
    case VisualModel::Volley:
        chosenPrefab = &m_visualVolleyPrefab;
        break;
    }

    if (chosenPrefab != nullptr && chosenPrefab->m_id.isValid())
    {
		// instance as child of GameObject Projectile, so it moves with it and we can destroy it when the projectile is returned to the pool
        m_visualGO = GameObjectAPI::instantiatePrefab(chosenPrefab->m_id, Vector3::Zero, Vector3::Zero, getOwner());
        if (m_visualGO != nullptr)
        {
			// making sure the visual model is at the same position and rotation as the projectile, so it doesn't appear offset
            Transform* visTrans = GameObjectAPI::getTransform(m_visualGO);
            Transform* projTrans = GameObjectAPI::getTransform(getOwner());
                if (visTrans != nullptr && projTrans != nullptr)
                {
                    // Ensure visual model matches projectile world transform. Use global setters to avoid
                    // incorrect local rotations when parent transforms have non-identity rotation/scale.
                    TransformAPI::setGlobalPosition(visTrans, TransformAPI::getGlobalPosition(projTrans));
                    TransformAPI::setGlobalRotationEuler(visTrans, TransformAPI::getGlobalEulerDegrees(projTrans));
                    ParticleLifecycle::restart(m_visualGO); // if the prefab has particle systems, restart them to ensure they play from the beginning
                }
        }
    }

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

    // destruir visual si existe
    if (m_visualGO != nullptr)
    {
        GameObjectAPI::removeGameObject(m_visualGO);
        m_visualGO = nullptr;
    }

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
