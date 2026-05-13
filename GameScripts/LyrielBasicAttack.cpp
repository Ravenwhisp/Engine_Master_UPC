#include "pch.h"
#include "LyrielBasicAttack.h"

#include "LyrielCharacter.h"
#include "CharacterBase.h"
#include "PlayerTargetController.h"
#include "PlayerState.h"
#include "PlayerRotation.h"
#include "ArrowPool.h"
#include "LyrielArrowProjectile.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(LyrielBasicAttack, LyrielAbilityBase,
    SERIALIZED_FLOAT(m_attackDamage, "Attack Damage", 0.0f, 100.0f, 0.5f),
    SERIALIZED_FLOAT(m_arrowSpeed, "Arrow Speed", 0.0f, 100.0f, 0.5f),
    SERIALIZED_FLOAT(m_attackLockDuration, "Attack Lock Duration", 0.0f, 2.0f, 0.01f)
)

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

    beginAttackPresentation();

    beginAttackWindow(m_attackLockDuration);
    startCooldown();

    Debug::log("[LyrielBasicAttack] Shot arrow to target '%s'.", GameObjectAPI::getName(target));
}

bool LyrielBasicAttack::spawnArrowToTarget(GameObject* target)
{
    if (m_lyrielCharacter == nullptr || target == nullptr)
    {
        return false;
    }

    ArrowPool* arrowPool = m_lyrielCharacter->getArrowPool();
    if (arrowPool == nullptr)
    {
        return false;
    }

    LyrielArrowProjectile* arrow = arrowPool->acquireArrow();
    if (arrow == nullptr)
    {
        return false;
    }

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

    const float arrowLifetime = distance / m_arrowSpeed;
    arrow->launch(startPosition, direction, m_arrowSpeed, arrowLifetime, target, m_attackDamage);

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

IMPLEMENT_SCRIPT(LyrielBasicAttack)