#include "pch.h"
#include "HealthPickup.h"
#include "PlayerDamageable.h"
#include "CooperativeSound.h"

#include <cmath>


IMPLEMENT_SCRIPT_FIELDS_INHERITED(HealthPickup, Pickup,
    SERIALIZED_FLOAT(m_healAmount, "Heal Amount",         0.0f, 100.0f, 1.0f),
    SERIALIZED_ASSET_REF(m_collectParticlePrefab, "Collect Particle Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(m_spawnHeight, "Spawn Height",        0.0f,   5.0f, 0.1f),
    SERIALIZED_FLOAT(m_fallGravity, "Fall Gravity",        0.0f,  20.0f, 0.5f),
    SERIALIZED_FLOAT(m_idleSpeed, "Idle Speed",          0.0f,  10.0f, 0.05f),
    SERIALIZED_FLOAT(m_horizontalAmplitude, "Horizontal Amplitude",0.0f,   3.0f, 0.05f),
    SERIALIZED_FLOAT(m_verticalAmplitude, "Vertical Amplitude",  0.0f,   3.0f, 0.05f),
)

HealthPickup::HealthPickup(GameObject* owner)
    : Pickup(owner)
{
}

void HealthPickup::Start()
{
    Transform* t        = GameObjectAPI::getTransform(getOwner());
    m_fallStartPosition = TransformAPI::getGlobalPosition(t);  // already at arc origin

    m_startPosition = m_hasCustomSpawnFrom
        ? m_landingPosition
        : Vector3(m_fallStartPosition.x, m_fallStartPosition.y - m_spawnHeight, m_fallStartPosition.z);

    // Precompute constant horizontal velocity so XZ reaches target exactly when Y lands
    const float fallHeight = m_fallStartPosition.y - m_startPosition.y;
    if (fallHeight > 0.0f && m_fallGravity > 0.0f)
    {
        const float estimatedTime = std::sqrt(2.0f * fallHeight / m_fallGravity);
        if (estimatedTime > 0.0f)
        {
            m_fallHVelocityX = (m_startPosition.x - m_fallStartPosition.x) / estimatedTime;
            m_fallHVelocityZ = (m_startPosition.z - m_fallStartPosition.z) / estimatedTime;
        }
    }

    m_isFalling    = true;
    m_fallVelocity = 0.0f;

    const auto coopGOs = SceneAPI::findAllGameObjectsWithScript<CooperativeSound>();
    if (!coopGOs.empty())
    {
        m_cooperativeSound = GameObjectAPI::findScript<CooperativeSound>(coopGOs.front());
    }
}

void HealthPickup::Update()
{
    if (m_collected)
    {
        return;
    }

    if (m_isFalling)
    {
        fallAnimation();
    }
    else
    {
        idleAnimation();
    }
}
void HealthPickup::OnTriggerEnter(GameObject* player)
{
    Debug::log("HealthPickup triggered by %s", GameObjectAPI::getName(player));

    if (m_collected)
    {
        return;
    }

    if (!player || GameObjectAPI::getTag(player) != Tag::PLAYER)
    {
        return;
    }

    Damageable* damageable = GameObjectAPI::findScript<Damageable>(player);

    if (!damageable || damageable->isDead())
    {
        return;
    }

    if (damageable->getCurrentHp() >= damageable->getMaxHp())
    {
        return;
    }

    Debug::log("Player %s can collect health pickup, healing for %f", GameObjectAPI::getName(player), m_healAmount);

    damageable->heal(m_healAmount);

    if (m_cooperativeSound != nullptr)
    {
        m_cooperativeSound->playHealthOrb();
    }

    if (!m_collectParticlePrefabPath.empty())
    {
        Transform* t = GameObjectAPI::getTransform(getOwner());
        Vector3 spawnPosition = t != nullptr ? TransformAPI::getGlobalPosition(t) : Vector3::Zero;
        GameObjectAPI::instantiatePrefab(m_collectParticlePrefab.m_ref, spawnPosition, Vector3::Zero, nullptr);
    }

    Pickup::OnTriggerEnter(player);
}

void HealthPickup::setupDrop(float healAmount, const Vector3& landingPosition)
{
    m_healAmount = healAmount;
    m_landingPosition = landingPosition;
    m_hasCustomSpawnFrom = true;
}

void HealthPickup::fallAnimation()
{
    const float dt = Time::getDeltaTime();
    m_fallVelocity += m_fallGravity * dt;

    Transform* t = GameObjectAPI::getTransform(getOwner());
    Vector3 pos  = TransformAPI::getGlobalPosition(t);

    pos.x += m_fallHVelocityX * dt;
    pos.z += m_fallHVelocityZ * dt;
    pos.y -= m_fallVelocity    * dt;

    if (pos.y <= m_startPosition.y)
    {
        pos            = m_startPosition;
        m_isFalling    = false;
        m_fallVelocity = 0.0f;
    }

    TransformAPI::setGlobalPosition(t, pos);
}

void HealthPickup::idleAnimation()
{
    m_idleTimer += Time::getDeltaTime();

    const float t = m_idleTimer * m_idleSpeed;

    Vector3 position = m_startPosition;

    position.z += std::sin(t) * m_horizontalAmplitude;
    position.y = m_startPosition.y + std::abs(std::sin(t * 2.0f)) * m_verticalAmplitude;

    TransformAPI::setGlobalPosition(GameObjectAPI::getTransform(getOwner()), position);
}


IMPLEMENT_SCRIPT(HealthPickup)
