#include "pch.h"
#include "PaladinVFX.h"

IMPLEMENT_SCRIPT_FIELDS(PaladinVFX,
    SERIALIZED_ASSET_REF(m_walkingDustPrefab, "Walking Dust Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_chargeAttackEffectPrefab, "Charge Attack Effect Prefab", AssetType::PREFAB),
    SERIALIZED_ASSET_REF(m_basicAttackEffectPrefab, "Basic Attack Effect Prefab", AssetType::PREFAB),
    SERIALIZED_FLOAT(walkingDustYOffset, "Walking Dust Y Offset", -5.0f, 5.0f, 0.05f),
    SERIALIZED_FLOAT(walkingDustForwardOffset, "Walking Dust Forward Offset", -5.0f, 5.0f, 0.05f)
)

PaladinVFX::PaladinVFX(GameObject* owner)
    : Script(owner)
{
}

void PaladinVFX::Start()
{
    walkingDustEffect = nullptr;
    walkingDustActive = false;

    chargeAttackEffect = nullptr;
    chargeAttackEffectActive = false;

    basicAttackEffect = nullptr;
    basicAttackEffectTimer = 0.0f;
}

void PaladinVFX::Update()
{
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

void PaladinVFX::playBasicAttackEffect()
{
    removeBasicAttackEffect();

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

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    const Vector3 ownerForward = TransformAPI::getForward(ownerTransform);

    return Vector3(
        ownerPosition.x + ownerForward.x * walkingDustForwardOffset,
        ownerPosition.y + walkingDustYOffset,
        ownerPosition.z + ownerForward.z * walkingDustForwardOffset
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

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    const Vector3 ownerForward = TransformAPI::getForward(ownerTransform);

    return Vector3(
        ownerPosition.x + ownerForward.x * chargeAttackForwardOffset,
        ownerPosition.y + chargeAttackYOffset,
        ownerPosition.z + ownerForward.z * chargeAttackForwardOffset
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

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    const Vector3 ownerForward = TransformAPI::getForward(ownerTransform);

    return Vector3(
        ownerPosition.x + ownerForward.x * basicAttackForwardOffset,
        ownerPosition.y + basicAttackYOffset,
        ownerPosition.z + ownerForward.z * basicAttackForwardOffset
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

void PaladinVFX::addWalkingDust()
{
    removeWalkingDust();

    walkingDustEffect = GameObjectAPI::instantiatePrefab(
        m_walkingDustPrefab.m_ref,
        getWalkingDustPosition(),
        getOwnerRotation()
    );

    if (!walkingDustEffect)
    {
        Debug::warn("[PaladinVFX] Could not instantiate WalkingDust prefab.");
        walkingDustActive = false;
    }
}

void PaladinVFX::removeWalkingDust()
{
    if (!walkingDustEffect)
    {
        return;
    }

    GameObjectAPI::removeGameObject(walkingDustEffect);
    walkingDustEffect = nullptr;
}

void PaladinVFX::updateWalkingDustPosition()
{
    Transform* walkingDustTransform = GameObjectAPI::getTransform(walkingDustEffect);

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
    removeChargeAttackEffect();

    chargeAttackEffect = GameObjectAPI::instantiatePrefab(
        m_chargeAttackEffectPrefab.m_ref,
        getChargeAttackEffectPosition(),
        getOwnerRotation()
    );

    if (!chargeAttackEffect)
    {
        Debug::warn("[PaladinVFX] Could not instantiate ChargeAttackEffect prefab.");
        chargeAttackEffectActive = false;
    }
}

void PaladinVFX::removeChargeAttackEffect()
{
    if (!chargeAttackEffect)
    {
        return;
    }

    GameObjectAPI::removeGameObject(chargeAttackEffect);
    chargeAttackEffect = nullptr;
}

void PaladinVFX::updateChargeAttackEffectPosition()
{
    Transform* chargeAttackEffectTransform = GameObjectAPI::getTransform(chargeAttackEffect);

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

void PaladinVFX::addBasicAttackEffect()
{
    basicAttackEffect = GameObjectAPI::instantiatePrefab(
        m_basicAttackEffectPrefab.m_ref,
        getBasicAttackEffectPosition(),
        getOwnerRotation()
    );

    if (!basicAttackEffect)
    {
        Debug::warn("[PaladinVFX] Could not instantiate BasicAttackEffect prefab.");
    }
}

void PaladinVFX::removeBasicAttackEffect()
{
    if (!basicAttackEffect)
    {
        return;
    }

    GameObjectAPI::removeGameObject(basicAttackEffect);
    basicAttackEffect = nullptr;
}

void PaladinVFX::updateBasicAttackEffectLifetime(float deltaTime)
{
    if (!basicAttackEffect)
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