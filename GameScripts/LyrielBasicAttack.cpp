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
#include "LyrielConfig.h"

LyrielBasicAttack::LyrielBasicAttack(GameObject* owner)
    : LyrielAbilityBase(owner)
{
}

void LyrielBasicAttack::Start()
{
    LyrielAbilityBase::Start();
}

void LyrielBasicAttack::Update()
{
	LyrielAbilityBase::Update();
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
    if (target == nullptr)
    {
        Debug::log("[LyrielBasicAttack] No current target.");
        return;
    }

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

IMPLEMENT_SCRIPT(LyrielBasicAttack)