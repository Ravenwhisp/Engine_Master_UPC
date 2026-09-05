#include "pch.h"
#include "ArthurParticles.h"
#include "ParticleLifecycle.h"
#include "EnemyDetectionAggro.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(ArthurParticles,
    SERIALIZED_STRING(m_groundDustPath, "Ground Dust Prefab Path"),
    SERIALIZED_ASSET_REF(m_groundDustPrefab, "Ground Dust Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_chargingSlamPath, "Charging Slam Prefab Path"),
    SERIALIZED_ASSET_REF(m_chargingSlamPrefab, "Charging Slam Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_earthHammerShockwavePath, "Earth Hammer Shockwave Prefab Path"),
    SERIALIZED_ASSET_REF(m_earthHammerShockwavePrefab, "Earth Hammer Shockwave Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_heavySwipePath, "Heavy Swipe Prefab Path"),
    SERIALIZED_ASSET_REF(m_heavySwipePrefab, "Heavy Swipe Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(m_chargingSlamGroundDustDelay, "Charging Slam Ground Dust Delay", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_chargingSlamExtraDelay, "Charging Slam Extra Delay", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_earthHammerGroundDustDelay, "Earth Hammer Ground Dust Delay", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_earthHammerShockwaveDelay, "Earth Hammer Shockwave Delay", 0.0f, 10.0f, 0.05f),
    SERIALIZED_FLOAT(m_heavySwipeHitDelay, "Heavy Swipe Hit Delay", 0.0f, 10.0f, 0.05f)
)

ArthurParticles::ArthurParticles(GameObject* owner)
    : Script(owner)
{
}

void ArthurParticles::Start()
{
    m_ownerTransform = GameObjectAPI::getTransform(getOwner());
    m_detectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
}

void ArthurParticles::OnGameStop()
{
    m_timedEffects.clear();
    m_timedOneShots.clear();
    ParticleLifecycle::destroy(m_earthHammerShockwaveInstance);
    m_earthHammerShockwaveActive = false;
}

void ArthurParticles::Update()
{
    processTimedEffects(Time::getDeltaTime());
    m_timedOneShots.update(Time::getDeltaTime());

    if (m_earthHammerShockwaveActive && m_earthHammerShockwaveInstance != nullptr)
    {
        ParticleLifecycle::syncToTransform(m_earthHammerShockwaveInstance, m_ownerTransform);
    }
}

void ArthurParticles::scheduleEffect(TimedEffect::Type type, const Vector3& position, float delay)
{
    TimedEffect effect;
    effect.type = type;
    effect.timer = delay;
    effect.position = position;
    m_timedEffects.push_back(effect);
}

void ArthurParticles::processTimedEffects(float deltaTime)
{
    for (size_t i = 0; i < m_timedEffects.size();)
    {
        TimedEffect& effect = m_timedEffects[i];
        effect.timer -= deltaTime;

        if (effect.timer > 0.0f)
        {
            ++i;
            continue;
        }

        switch (effect.type)
        {
        case TimedEffect::Type::GroundDust:
            spawnGroundDust(effect.position);
            break;
        case TimedEffect::Type::ChargingSlam:
            spawnChargingSlam(effect.position);
            break;
        case TimedEffect::Type::HeavySwipeHit:
            spawnHeavySwipeHit(effect.position);
            break;
        case TimedEffect::Type::ActivateShockwave:
            activateEarthHammerShockwave();
            break;
        case TimedEffect::Type::DeactivateShockwave:
            deactivateEarthHammerShockwave();
            break;
        }

        m_timedEffects.erase(m_timedEffects.begin() + static_cast<std::ptrdiff_t>(i));
    }
}

void ArthurParticles::spawnGroundDust(const Vector3& position)
{
    ParticleLifecycle::spawnOneShotTimed(
        m_timedOneShots,
        m_groundDustPrefab.m_id,
        position,
        Vector3::Zero,
        ParticleLifecycle::kDefaultOneShotLifetime
    );
}

void ArthurParticles::spawnChargingSlam(const Vector3& position)
{
    ParticleLifecycle::spawnOneShotTimed(
        m_timedOneShots,
        m_chargingSlamPrefab.m_id,
        position,
        Vector3::Zero,
        ParticleLifecycle::kDefaultOneShotLifetime
    );
}

void ArthurParticles::spawnHeavySwipeHit(const Vector3& position)
{
    ParticleLifecycle::spawnOneShotTimed(
        m_timedOneShots,
        m_heavySwipePrefab.m_id,
        position,
        Vector3::Zero,
        ParticleLifecycle::kDefaultOneShotLifetime
    );
}

void ArthurParticles::activateEarthHammerShockwave()
{
    if (m_ownerTransform == nullptr)
    {
        m_ownerTransform = GameObjectAPI::getTransform(getOwner());
    }

    const Vector3 position = m_ownerTransform != nullptr ? TransformAPI::getGlobalPosition(m_ownerTransform) : Vector3::Zero;
    const Vector3 rotation = m_ownerTransform != nullptr ? TransformAPI::getGlobalEulerDegrees(m_ownerTransform) : Vector3::Zero;

    ParticleLifecycle::ensurePersistent(m_earthHammerShockwaveInstance, m_earthHammerShockwavePrefab.m_id, position, rotation, nullptr);
    ParticleLifecycle::syncToTransform(m_earthHammerShockwaveInstance, m_ownerTransform);
    ParticleLifecycle::activate(m_earthHammerShockwaveInstance);
    m_earthHammerShockwaveActive = m_earthHammerShockwaveInstance != nullptr;
}

void ArthurParticles::deactivateEarthHammerShockwave()
{
    ParticleLifecycle::deactivate(m_earthHammerShockwaveInstance);
    m_earthHammerShockwaveActive = false;
}

void ArthurParticles::playChargingSlamImpact(const Vector3& position)
{
    scheduleEffect(TimedEffect::Type::GroundDust, position, m_chargingSlamGroundDustDelay);
    scheduleEffect(TimedEffect::Type::ChargingSlam, position, m_chargingSlamExtraDelay);
}

void ArthurParticles::startEarthHammerShockwave()
{
    scheduleEffect(TimedEffect::Type::ActivateShockwave, Vector3::Zero, m_earthHammerShockwaveDelay);
}

void ArthurParticles::stopEarthHammerShockwave()
{
    deactivateEarthHammerShockwave();
}

void ArthurParticles::playEarthHammerImpact(const Vector3& position)
{
    scheduleEffect(TimedEffect::Type::GroundDust, position, m_earthHammerGroundDustDelay);
}

bool ArthurParticles::isTargetInCone(Transform* targetTransform, const Vector3& center, const Vector3& forward, float range, float halfAngleDegrees) const
{
    if (targetTransform == nullptr)
    {
        return false;
    }

    Vector3 toTarget = TransformAPI::getGlobalPosition(targetTransform) - center;
    toTarget.y = 0.0f;

    const float distanceSquared = toTarget.LengthSquared();
    if (distanceSquared > range * range || distanceSquared <= 0.0001f)
    {
        return false;
    }

    Vector3 flatForward = forward;
    flatForward.y = 0.0f;
    if (flatForward.LengthSquared() <= 0.0001f)
    {
        return false;
    }

    flatForward.Normalize();
    toTarget.Normalize();

    const float dot = flatForward.x * toTarget.x + flatForward.z * toTarget.z;
    const float angleRadians = std::acos(std::clamp(dot, -1.0f, 1.0f));
    const float halfAngleRadians = halfAngleDegrees * (3.14159265f / 180.0f);

    return angleRadians <= halfAngleRadians;
}

void ArthurParticles::playHeavySwipeHitsInCone(const Vector3& center, const Vector3& forward, float range, float halfAngleDegrees, int hitCount)
{
    if (hitCount <= 0 || m_detectionAggro == nullptr)
    {
        return;
    }

    Transform* lyrielTransform = m_detectionAggro->getLyrielTransform();
    Transform* deathTransform = m_detectionAggro->getDeathTransform();

    if (isTargetInCone(lyrielTransform, center, forward, range, halfAngleDegrees))
    {
        scheduleEffect(TimedEffect::Type::HeavySwipeHit, TransformAPI::getGlobalPosition(lyrielTransform), m_heavySwipeHitDelay);
    }

    if (isTargetInCone(deathTransform, center, forward, range, halfAngleDegrees))
    {
        scheduleEffect(TimedEffect::Type::HeavySwipeHit, TransformAPI::getGlobalPosition(deathTransform), m_heavySwipeHitDelay);
    }
}

IMPLEMENT_SCRIPT(ArthurParticles)
