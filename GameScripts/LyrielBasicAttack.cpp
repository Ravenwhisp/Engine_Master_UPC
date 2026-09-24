#include "pch.h"
#include "LyrielBasicAttack.h"

#include "LyrielCharacter.h"
#include "LyrielSound.h"
#include "CharacterBase.h"
#include "PlayerTargetController.h"
#include "PlayerState.h"
#include "PlayerRotation.h"
#include "ProjectilePool.h"
#include "LyrielArrowProjectile.h"
#include "LyrielUI.h"
#include "LyrielConfig.h"
#include "PlayerRotation.h"

LyrielBasicAttack::LyrielBasicAttack(GameObject* owner)
    : LyrielAbilityBase(owner)
{
}

void LyrielBasicAttack::Start()
{
    LyrielAbilityBase::Start();

    m_lyrielUI = GameObjectAPI::findScript<LyrielUI>(getOwner());

    if (!m_lyrielUI)
    {
        Debug::warn("[LyrielBasicAttack] LyrielUI not found.");
    }
}

void LyrielBasicAttack::Update()
{
    LyrielAbilityBase::Update();

    if (m_isAiming)
    {
        if (Input::isRightShoulderPressed(getPlayerIndex()))
        {
            updateAim();
        }
        else
        {
            releaseAimAndCast();
        }
    }
}

void LyrielBasicAttack::onAttackWindowUpdate()
{
    if (m_attackFacingTarget != nullptr)
    {
        faceTarget(m_attackFacingTarget);
    }
}

void LyrielBasicAttack::onAttackWindowFinished()
{
    m_attackFacingTarget = nullptr;
}

void LyrielBasicAttack::startAbility()
{
    PlayerTargetController* targetController = m_character->getTargetController();
    if (targetController == nullptr)
    {
        return;
    }

    GameObject* target = targetController->getCurrentTarget();

    if (target != nullptr)
    {
        setAbilityLocked(true);

        faceTarget(target);
        m_attackFacingTarget = target;

        if (!spawnArrowToTarget(target))
        {
            setAbilityLocked(false);
            m_attackFacingTarget = nullptr;
            return;
        }

        notifyAbilitySuccessfullyStarted();

        LyrielSound* sound = m_lyrielCharacter != nullptr ? m_lyrielCharacter->getSound() : nullptr;
        if (sound != nullptr)
        {
            sound->playBowRelease();
        }

        beginAttackPresentation();

        beginAttackWindow(m_lyrielCharacter->getConfig()->m_basicAttackLockDuration);
        startCooldown();

        Debug::log("[LyrielBasicAttack] Shot arrow to target '%s'.", GameObjectAPI::getName(target));
    }
    else
    {
        beginAim();
    }
}

bool LyrielBasicAttack::spawnArrowToTarget(GameObject* target)
{
    if (m_lyrielCharacter == nullptr || target == nullptr)
    {
        return false;
    }

    ProjectilePool* projectilePool = m_lyrielCharacter->getArrowPool();
    if (!projectilePool)
    {
        return false;
    }

    ProjectileBase* projectile = projectilePool->acquireProjectile();
    if (!projectile)
    {
        return false;
    }

    LyrielArrowProjectile* arrow = static_cast<LyrielArrowProjectile*>(projectile);

    Transform* spawnTransform = findArrowSpawnTransform();
    Transform* targetTransform = GameObjectAPI::getTransform(target);

    if (spawnTransform == nullptr || targetTransform == nullptr)
    {
        return false;
    }

    const Vector3 startPosition = TransformAPI::getGlobalPosition(spawnTransform);
    const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    Vector3 direction = targetPosition - startPosition;
    const float distance = direction.Length();

    if (distance <= 0.0001f)
    {
        direction = TransformAPI::getForward(spawnTransform);
    }
    else
    {
        direction.Normalize();
    }

    const float arrowLifetime = distance / m_lyrielCharacter->getConfig()->m_basicArrowSpeed;
    arrow->launch(startPosition, direction, m_lyrielCharacter->getConfig()->m_basicArrowSpeed, arrowLifetime, target, m_lyrielCharacter->getConfig()->m_basicAttackDamage);

    return true;
}

bool LyrielBasicAttack::spawnArrowToDirection(const Vector3& direction)
{
    if (m_lyrielCharacter == nullptr)
    {
        return false;
    }

    ProjectilePool* projectilePool = m_lyrielCharacter->getArrowPool();
    if (!projectilePool)
    {
        return false;
    }

    ProjectileBase* projectile = projectilePool->acquireProjectile();
    if (!projectile)
    {
        return false;
    }

    LyrielArrowProjectile* arrow = static_cast<LyrielArrowProjectile*>(projectile);

    Transform* spawnTransform = findArrowSpawnTransform();
    if (spawnTransform == nullptr)
    {
        return false;
    }

    const Vector3 startPosition = TransformAPI::getGlobalPosition(spawnTransform);

    const float range = m_lyrielCharacter->getConfig()->m_basicAimArrowRange;
    const float arrowLifetime = range / m_lyrielCharacter->getConfig()->m_basicArrowSpeed;
    arrow->launch(startPosition, direction, m_lyrielCharacter->getConfig()->m_basicArrowSpeed, arrowLifetime, nullptr, m_lyrielCharacter->getConfig()->m_basicAttackDamage);

    return true;
}

void LyrielBasicAttack::faceTarget(GameObject* target)
{
    if (m_character == nullptr || target == nullptr)
    {
        return;
    }

    PlayerRotation* playerRotation = m_character->getPlayerRotation();
    if (playerRotation == nullptr)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    Transform* targetTransform = GameObjectAPI::getTransform(target);

    if (ownerTransform == nullptr || targetTransform == nullptr)
    {
        return;
    }

    const Vector3 ownerPosition = TransformAPI::getGlobalPosition(ownerTransform);
    const Vector3 targetPosition = TransformAPI::getGlobalPosition(targetTransform);

    Vector3 direction = targetPosition - ownerPosition;
    direction.y = 0.0f;

    if (direction.LengthSquared() <= 0.0001f)
    {
        return;
    }

    direction.Normalize();
    playerRotation->applyFacingFromDirection(getOwner(), direction, Time::getDeltaTime());
}

float LyrielBasicAttack::getCooldown() const
{
    return m_lyrielCharacter->getConfig()->m_basicCooldown;
}

bool LyrielBasicAttack::canCast() const
{
    return m_character != nullptr && !m_character->isDowned();
}

void LyrielBasicAttack::beginAim()
{
    m_isAiming = true;
    setAbilityLocked(true);

    Vector3 aimDirection = computeAimDirection();
    m_currentAimDirection = isAimStickValid(aimDirection) ? aimDirection : getFallbackFacingDirection();

    faceDirection(m_currentAimDirection);

    if (m_lyrielUI)
    {
        m_lyrielUI->showBasicAttackUI();
        updateAimUI();
    }
}

void LyrielBasicAttack::updateAim()
{
    Vector3 aimDirection = computeAimDirection();
    if (isAimStickValid(aimDirection))
    {
        m_currentAimDirection = aimDirection;
        faceDirection(m_currentAimDirection);
    }

    updateAimUI();
}

void LyrielBasicAttack::updateAimUI()
{
    if (!m_lyrielUI)
    {
        return;
    }

    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr)
    {
        return;
    }

    const Vector3 origin = TransformAPI::getGlobalPosition(ownerTransform);

    Vector3 facing = TransformAPI::getForward(ownerTransform);
    facing.y = 0.0f;

    if (facing.LengthSquared() <= 0.0001f)
    {
        facing = m_currentAimDirection;
    }

    m_lyrielUI->updateBasicAttackUI(origin, facing);
}

void LyrielBasicAttack::releaseAimAndCast()
{
    m_isAiming = false;

    if (m_lyrielUI)
    {
        m_lyrielUI->hideBasicAttackUI();
    }

    if (!canCast())
    {
        setAbilityLocked(false);
        return;
    }

    Vector3 forward = m_currentAimDirection;
    if (!isAimStickValid(forward))
    {
        forward = getFallbackFacingDirection();
    }

    if (forward.LengthSquared() <= 0.0001f)
    {
        setAbilityLocked(false);
        return;
    }

    faceDirection(forward);

    if (!spawnArrowToDirection(forward))
    {
        setAbilityLocked(false);
        return;
    }

    notifyAbilitySuccessfullyStarted();

    LyrielSound* sound = m_lyrielCharacter != nullptr ? m_lyrielCharacter->getSound() : nullptr;
    if (sound != nullptr)
    {
        sound->playBowRelease();
    }

    beginAttackPresentation();

    beginAttackWindow(m_lyrielCharacter->getConfig()->m_basicAttackLockDuration);
    startCooldown();

    Debug::log("[LyrielBasicAttack] Aimed shot released.");
}

Vector3 LyrielBasicAttack::computeAimDirection() const
{
    return computeCameraRelativeAimDirection();
}

bool LyrielBasicAttack::isAimStickValid(const Vector3& direction) const
{
    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;
    return flatDirection.LengthSquared() > 0.0001f;
}

IMPLEMENT_SCRIPT(LyrielBasicAttack)