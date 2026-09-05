#include "pch.h"
#include "PaladinVFX.h"
#include "ParticleLifecycle.h"
#include "EnemyDetectionAggro.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS(PaladinVFX,
    SERIALIZED_ASSET_REF(m_walkingDustPrefab, "Walking Dust Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_chargeAttackEffectPrefab, "Charge Attack Effect Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_basicAttackEffectPrefab, "Basic Attack Effect Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_shieldAttackParticlesPath, "Shield Attack Particles Prefab Path"),
    SERIALIZED_ASSET_REF(m_shieldAttackParticlesPrefab, "Shield Attack Particles Prefab", AssetType::PREFAB),
    SERIALIZED_STRING(m_shieldAttackHitPath, "Shield Attack Hit Prefab Path"),
    SERIALIZED_ASSET_REF(m_shieldAttackHitPrefab, "Shield Attack Hit Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(walkingDustYOffset, "Walking Dust Y Offset", -5.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(walkingDustForwardOffset, "Walking Dust Forward Offset", -5.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(m_shieldAttackParticlesYOffset, "Shield Attack Particles Y Offset", -5.0f, 5.0f, 0.05f)
)

PaladinVFX::PaladinVFX(GameObject* owner)
    : Script(owner)
{
}

void PaladinVFX::Start()
{
    walkingDustActive = false;
    chargeAttackEffectActive = false;
    basicAttackEffectTimer = 0.0f;
    m_detectionAggro = GameObjectAPI::findScript<EnemyDetectionAggro>(getOwner());
}

void PaladinVFX::OnGameStop()
{
    m_timedHitVfx.clear();
    ParticleLifecycle::destroy(walkingDustEffect);
    ParticleLifecycle::destroy(chargeAttackEffect);
    ParticleLifecycle::destroy(basicAttackTelegraph);
    ParticleLifecycle::destroy(basicAttackEffect);
}

void PaladinVFX::Update()
{
    m_timedHitVfx.update(Time::getDeltaTime());

    if (walkingDustActive && walkingDustEffect)
    {
        updateWalkingDustPosition();
    }

    if (chargeAttackEffectActive && chargeAttackEffect)
    {
        updateChargeAttackEffectPosition();
    }

    updateBasicAttackEffectLifetime(Time::getDeltaTime());
}

void PaladinVFX::setWalkingDustActive(bool active)
{
    if (walkingDustActive == active)
    {
        return;
    }

    walkingDustActive = active;

    if (walkingDustActive)
    {
        addWalkingDust();
    }
    else
    {
        removeWalkingDust();
    }
}

void PaladinVFX::startChargeAttackEffect()
{
    if (chargeAttackEffectActive)
    {
        return;
    }

    chargeAttackEffectActive = true;
    addChargeAttackEffect();
}

void PaladinVFX::stopChargeAttackEffect()
{
    chargeAttackEffectActive = false;
    removeChargeAttackEffect();
}

void PaladinVFX::startBasicAttackTelegraph(
    const Vector3& position,
    const Vector3& rotation
)
{
    addBasicAttackTelegraph(position, rotation);
}

void PaladinVFX::stopBasicAttackTelegraph()
{
    removeBasicAttackTelegraph();
}

void PaladinVFX::playBasicAttackEffect()
{
    addBasicAttackEffect();

    if (basicAttackEffect)
    {
        basicAttackEffectTimer = basicAttackEffectLifetime;
    }
    else
    {
        basicAttackEffectTimer = 0.0f;
    }
}

void PaladinVFX::playShieldAttackStart(const Vector3& position, const Vector3& direction)
{
    if (!m_shieldAttackParticlesPrefab.m_id.isValid())
    {
        return;
    }

    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    Vector3 rotation = Vector3::Zero;
    if (flatDirection.LengthSquared() > 0.0001f)
    {
        flatDirection.Normalize();
        const float yawDegrees = std::atan2(flatDirection.x, flatDirection.z) * (180.0f / 3.14159265f);
        rotation = Vector3(0.0f, yawDegrees, 0.0f);
    }

    Vector3 spawnPosition = position;
    spawnPosition.y += m_shieldAttackParticlesYOffset;

    GameObject* instance = GameObjectAPI::instantiatePrefab(m_shieldAttackParticlesPrefab.m_id, spawnPosition, rotation, nullptr);
    if (instance != nullptr)
    {
        m_timedHitVfx.scheduleDestroy(instance, ParticleLifecycle::kDefaultOneShotLifetime);
    }
}

void PaladinVFX::spawnShieldAttackHit(const Vector3& position)
{
    ParticleLifecycle::spawnOneShotTimed(
        m_timedHitVfx,
        m_shieldAttackHitPrefab.m_id,
        position,
        Vector3::Zero,
        ParticleLifecycle::kDefaultOneShotLifetime
    );
}

bool PaladinVFX::isTargetInRectangle(
    Transform* targetTransform,
    const Vector3& origin,
    const Vector3& direction,
    float length,
    float width
) const
{
    if (!targetTransform || length <= 0.0f || width <= 0.0f)
    {
        return false;
    }

    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() < 0.0001f)
    {
        return false;
    }

    flatDirection.Normalize();

    Vector3 rightDirection(flatDirection.z, 0.0f, -flatDirection.x);

    Vector3 toTarget = TransformAPI::getGlobalPosition(targetTransform) - origin;
    toTarget.y = 0.0f;

    const float forwardDistance = flatDirection.Dot(toTarget);

    if (forwardDistance < 0.0f || forwardDistance > length)
    {
        return false;
    }

    const float lateralDistance = rightDirection.Dot(toTarget);
    const float halfWidth = width * 0.5f;

    return lateralDistance >= -halfWidth && lateralDistance <= halfWidth;
}

void PaladinVFX::playShieldAttackHits(
    const Vector3& origin,
    const Vector3& direction,
    float length,
    float width
)
{
    if (!m_shieldAttackHitPrefab.m_id.isValid() || !m_detectionAggro)
    {
        return;
    }

    Transform* lyrielTransform = m_detectionAggro->getLyrielTransform();
    Transform* deathTransform = m_detectionAggro->getDeathTransform();

    if (isTargetInRectangle(lyrielTransform, origin, direction, length, width))
    {
        spawnShieldAttackHit(TransformAPI::getGlobalPosition(lyrielTransform));
    }

    if (isTargetInRectangle(deathTransform, origin, direction, length, width))
    {
        spawnShieldAttackHit(TransformAPI::getGlobalPosition(deathTransform));
    }
}

void PaladinVFX::stopWalkingDust()
{
    walkingDustActive = false;
    removeWalkingDust();
}

Vector3 PaladinVFX::getWalkingDustPosition() const
{
    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);

    if (!ownerTransform)
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    const Vector3 ownerPosition =
        TransformAPI::getGlobalPosition(ownerTransform);

    const Vector3 ownerForward =
        TransformAPI::getForward(ownerTransform);

    return Vector3(
        ownerPosition.x +
        ownerForward.x * walkingDustForwardOffset,
        ownerPosition.y + walkingDustYOffset,
        ownerPosition.z +
        ownerForward.z * walkingDustForwardOffset
    );
}

Vector3 PaladinVFX::getChargeAttackEffectPosition() const
{
    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);

    if (!ownerTransform)
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    const Vector3 ownerPosition =
        TransformAPI::getGlobalPosition(ownerTransform);

    const Vector3 ownerForward =
        TransformAPI::getForward(ownerTransform);

    return Vector3(
        ownerPosition.x +
        ownerForward.x * chargeAttackForwardOffset,
        ownerPosition.y + chargeAttackYOffset,
        ownerPosition.z +
        ownerForward.z * chargeAttackForwardOffset
    );
}

Vector3 PaladinVFX::getBasicAttackEffectPosition() const
{
    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);

    if (!ownerTransform)
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    const Vector3 ownerPosition =
        TransformAPI::getGlobalPosition(ownerTransform);

    const Vector3 ownerForward =
        TransformAPI::getForward(ownerTransform);

    return Vector3(
        ownerPosition.x +
        ownerForward.x * basicAttackForwardOffset,
        ownerPosition.y + basicAttackYOffset,
        ownerPosition.z +
        ownerForward.z * basicAttackForwardOffset
    );
}

Vector3 PaladinVFX::getOwnerRotation() const
{
    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);

    if (!ownerTransform)
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    return TransformAPI::getGlobalEulerDegrees(ownerTransform);
}

void PaladinVFX::ensureWalkingDust()
{
    ParticleLifecycle::ensurePersistent(
        walkingDustEffect,
        m_walkingDustPrefab.m_id,
        getWalkingDustPosition(),
        getOwnerRotation()
    );
}

void PaladinVFX::ensureChargeAttackEffect()
{
    ParticleLifecycle::ensurePersistent(
        chargeAttackEffect,
        m_chargeAttackEffectPrefab.m_id,
        getChargeAttackEffectPosition(),
        getOwnerRotation()
    );
}

void PaladinVFX::ensureBasicAttackTelegraph(const Vector3& position, const Vector3& rotation)
{
    ParticleLifecycle::ensurePersistent(
        basicAttackTelegraph,
        m_basicAttackEffectPrefab.m_id,
        position,
        rotation
    );
}

void PaladinVFX::ensureBasicAttackEffect()
{
    ParticleLifecycle::ensurePersistent(
        basicAttackEffect,
        m_basicAttackEffectPrefab.m_id,
        getBasicAttackEffectPosition(),
        getOwnerRotation()
    );
}

void PaladinVFX::addWalkingDust()
{
    ensureWalkingDust();

    if (!walkingDustEffect)
    {
        Debug::warn(
            "[PaladinVFX] Could not instantiate WalkingDust prefab."
        );

        walkingDustActive = false;
        return;
    }

    updateWalkingDustPosition();
    ParticleLifecycle::activate(walkingDustEffect);
}

void PaladinVFX::removeWalkingDust()
{
    ParticleLifecycle::deactivate(walkingDustEffect);
}

void PaladinVFX::updateWalkingDustPosition()
{
    if (!walkingDustEffect)
    {
        return;
    }

    Transform* walkingDustTransform =
        GameObjectAPI::getTransform(walkingDustEffect);

    if (!walkingDustTransform)
    {
        return;
    }

    TransformAPI::setGlobalPosition(
        walkingDustTransform,
        getWalkingDustPosition()
    );

    TransformAPI::setGlobalRotationEuler(
        walkingDustTransform,
        getOwnerRotation()
    );
}

void PaladinVFX::addChargeAttackEffect()
{
    ensureChargeAttackEffect();

    if (!chargeAttackEffect)
    {
        Debug::warn(
            "[PaladinVFX] Could not instantiate ChargeAttackEffect prefab."
        );

        chargeAttackEffectActive = false;
        return;
    }

    updateChargeAttackEffectPosition();
    ParticleLifecycle::activate(chargeAttackEffect);
}

void PaladinVFX::removeChargeAttackEffect()
{
    ParticleLifecycle::deactivate(chargeAttackEffect);
}

void PaladinVFX::updateChargeAttackEffectPosition()
{
    if (!chargeAttackEffect)
    {
        return;
    }

    Transform* chargeAttackEffectTransform =
        GameObjectAPI::getTransform(chargeAttackEffect);

    if (!chargeAttackEffectTransform)
    {
        return;
    }

    TransformAPI::setGlobalPosition(
        chargeAttackEffectTransform,
        getChargeAttackEffectPosition()
    );

    TransformAPI::setGlobalRotationEuler(
        chargeAttackEffectTransform,
        getOwnerRotation()
    );
}

void PaladinVFX::addBasicAttackTelegraph(
    const Vector3& position,
    const Vector3& rotation
)
{
    Vector3 spawnPosition = position;
    spawnPosition.y += basicAttackTelegraphYOffset;

    ensureBasicAttackTelegraph(spawnPosition, rotation);

    if (!basicAttackTelegraph)
    {
        Debug::warn(
            "[PaladinVFX] Could not instantiate BasicAttackTelegraph."
        );
        return;
    }

    Transform* telegraphTransform = GameObjectAPI::getTransform(basicAttackTelegraph);
    if (telegraphTransform)
    {
        TransformAPI::setGlobalPosition(telegraphTransform, spawnPosition);
        TransformAPI::setGlobalRotationEuler(telegraphTransform, rotation);
    }

    ParticleLifecycle::activate(basicAttackTelegraph);
}

void PaladinVFX::removeBasicAttackTelegraph()
{
    ParticleLifecycle::deactivate(basicAttackTelegraph);
}

void PaladinVFX::addBasicAttackEffect()
{
    ensureBasicAttackEffect();

    if (!basicAttackEffect)
    {
        Debug::warn(
            "[PaladinVFX] Could not instantiate BasicAttackEffect prefab."
        );
        return;
    }

    Transform* effectTransform = GameObjectAPI::getTransform(basicAttackEffect);
    if (effectTransform)
    {
        TransformAPI::setGlobalPosition(effectTransform, getBasicAttackEffectPosition());
        TransformAPI::setGlobalRotationEuler(effectTransform, getOwnerRotation());
    }

    ParticleLifecycle::activate(basicAttackEffect);
}

void PaladinVFX::removeBasicAttackEffect()
{
    ParticleLifecycle::deactivate(basicAttackEffect);
}

void PaladinVFX::updateBasicAttackEffectLifetime(float deltaTime)
{
    if (!basicAttackEffect || basicAttackEffectTimer <= 0.0f)
    {
        return;
    }

    basicAttackEffectTimer -= deltaTime;

    if (basicAttackEffectTimer <= 0.0f)
    {
        removeBasicAttackEffect();
        basicAttackEffectTimer = 0.0f;
    }
}

IMPLEMENT_SCRIPT(PaladinVFX)
