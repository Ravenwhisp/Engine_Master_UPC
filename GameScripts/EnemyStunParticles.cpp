#include "pch.h"
#include "EnemyStunParticles.h"
#include "ParticleLifecycle.h"
#include "Transform2D.h"

IMPLEMENT_SCRIPT_FIELDS(EnemyStunParticles,
    SERIALIZED_ASSET_REF(m_stunPrefab, "Stun Particle Prefab", AssetType::PREFAB),
    SERIALIZED_COMPONENT_REF(m_stunAnchor, "Stun Anchor", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_stunIcon, "Stun Sprite", ComponentType::TRANSFORM2D),
    SERIALIZED_FLOAT(m_iconFadeTime, "Stun Icon Fade Time", 0.0f, 1.0f, 0.01f),
    SERIALIZED_FLOAT(m_heightOffset, "Height Offset", 0.0f, 10.0f, 0.1f)
)

EnemyStunParticles::EnemyStunParticles(GameObject* owner) : Script(owner) {}

void EnemyStunParticles::Start()
{
    resolveStunIcon();
    applyStunIconAlpha();
}

void EnemyStunParticles::Update()
{
    if (!m_stunIconTransform || m_iconAlpha == m_iconTargetAlpha)
    {
        return;
    }

    m_iconAlpha = m_iconFadeTime > 0.0f
        ? MathAPI::moveTowards(m_iconAlpha, m_iconTargetAlpha, Time::getDeltaTime() / m_iconFadeTime)
        : m_iconTargetAlpha;
    applyStunIconAlpha();
}

void EnemyStunParticles::OnGameStop()
{
    releaseRuntimeParticles();
}

void EnemyStunParticles::releaseRuntimeParticles()
{
    stopStunParticle(true);
    ParticleLifecycle::destroy(m_stunParticle);
}

void EnemyStunParticles::ensureStunParticle()
{
    if (!m_stunAnchorTransform)
    {
        m_stunAnchorTransform = m_stunAnchor.getReferencedComponent();
        if (!m_stunAnchorTransform)
        {
            m_stunAnchorTransform = ParticleLifecycle::findChildRecursive(
                GameObjectAPI::getTransform(getOwner()), "Stun anchor");
        }
    }

    ParticleLifecycle::ensurePersistent(m_stunParticle, m_stunPrefab.m_id,
        getStunParticlePosition(), Vector3::Zero, getOwner());
}

Vector3 EnemyStunParticles::getStunParticlePosition() const
{
    if (m_stunAnchorTransform)
    {
        // Anchor placement replaces the fallback height; prefab child offsets still apply.
        return TransformAPI::getGlobalPosition(m_stunAnchorTransform);
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    Vector3 position = ownerTransform ? TransformAPI::getGlobalPosition(ownerTransform) : Vector3::Zero;
    position.y += m_heightOffset;
    return position;
}

void EnemyStunParticles::resolveStunIcon()
{
    if (m_stunIconTransform) 
    {
        return;
    }

    m_stunIconTransform = m_stunIcon.getReferencedComponent();
    if (m_stunIconTransform)
    {
        return;
    }

    Transform* healthBar = TransformAPI::findChildByName(GameObjectAPI::getTransform(getOwner()), "Health Bar");
    Transform* sprite = ParticleLifecycle::findChildRecursive(healthBar, "Stun sprite");
    if (sprite)
    {
        m_stunIconTransform = static_cast<Transform2D*>(GameObjectAPI::getComponent(ComponentAPI::getOwner(sprite), ComponentType::TRANSFORM2D));
    }
}

void EnemyStunParticles::applyStunIconAlpha()
{
    if (!m_stunIconTransform) return;

    Transform2DAPI::setAlpha(m_stunIconTransform, m_iconAlpha);
    GameObjectAPI::setActive(ComponentAPI::getOwner(m_stunIconTransform), m_iconTargetAlpha > 0.0f || m_iconAlpha > 0.0f);
}

void EnemyStunParticles::startStunParticle()
{
    ensureStunParticle();
    resolveStunIcon();
    updateStunParticle();
    ParticleLifecycle::activate(m_stunParticle);
    m_iconTargetAlpha = 1.0f;
    if (m_iconFadeTime <= 0.0f) m_iconAlpha = m_iconTargetAlpha;
    applyStunIconAlpha();
}

void EnemyStunParticles::updateStunParticle()
{
    if (!m_stunParticle) 
    {
        return;
    }

    Transform* particleTransform = GameObjectAPI::getTransform(m_stunParticle);
    if (particleTransform)
    {
        TransformAPI::setGlobalPosition(particleTransform, getStunParticlePosition());
    }
}

void EnemyStunParticles::stopStunParticle(bool immediate)
{
    ParticleLifecycle::deactivate(m_stunParticle);
    m_iconTargetAlpha = 0.0f;
    if (immediate || m_iconFadeTime <= 0.0f) m_iconAlpha = 0.0f;
    applyStunIconAlpha();
}

IMPLEMENT_SCRIPT(EnemyStunParticles)
