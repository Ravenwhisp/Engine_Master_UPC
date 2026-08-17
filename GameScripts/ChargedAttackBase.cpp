#include "pch.h"
#include "ChargedAttackBase.h"

#include "PlayerMovement.h"
#include "EnemyBaseController.h"

ChargedAttackBase::ChargedAttackBase(GameObject* owner)
    : AbilityBase(owner)
{
}

void ChargedAttackBase::Start()
{
    AbilityBase::Start();

    m_playerMovement = GameObjectAPI::findScript<PlayerMovement>(getOwner());

    if (m_playerMovement == nullptr)
    {
        Debug::warn("[ChargedAttackBase] PlayerMovement not found on owner '%s'.", GameObjectAPI::getName(getOwner()));
    }
}

void ChargedAttackBase::applyChargingMovementSlowdown(float slowdownPercentage)
{
    if (m_playerMovement == nullptr)
    {
        return;
    }

    const float movementMultiplier = 1.0f - slowdownPercentage / 100.0f;
    m_playerMovement->setMovementMultiplier(movementMultiplier);
}

void ChargedAttackBase::resetChargingMovementSlowdown()
{
    if (m_playerMovement == nullptr)
    {
        return;
    }

    m_playerMovement->resetMovementMultiplier();
}

void ChargedAttackBase::tryStunTarget(GameObject* target, bool isMaxCharge, bool stunEnabled, float stunDuration) const
{
    if (!stunEnabled || !isMaxCharge || target == nullptr)
    {
        return;
    }

    EnemyBaseController* enemyController = GameObjectAPI::findScript<EnemyBaseController>(target);

    if (enemyController != nullptr)
    {
        enemyController->useStun(stunDuration);
    }
}