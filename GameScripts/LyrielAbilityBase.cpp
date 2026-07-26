#include "pch.h"
#include "LyrielAbilityBase.h"

#include "LyrielCharacter.h"
#include "PlayerRotation.h"
#include "PlayerState.h"
#include "PlayerAnimationController.h"
#include "LyrielConfig.h"

LyrielAbilityBase::LyrielAbilityBase(GameObject* owner)
    : AbilityBase(owner)
{
}

void LyrielAbilityBase::Start()
{
    AbilityBase::Start();

    m_lyrielCharacter = dynamic_cast<LyrielCharacter*>(m_character);

}

Transform* LyrielAbilityBase::findArrowSpawnTransform() const
{
    Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
    if (ownerTransform == nullptr || m_lyrielCharacter == nullptr)
    {
        return nullptr;
    }

    if (!m_lyrielCharacter->m_arrowSpawnChildName.empty())
    {
        Transform* spawnTransform = TransformAPI::findChildByName(
            ownerTransform,
            m_lyrielCharacter->m_arrowSpawnChildName.c_str());

        if (spawnTransform != nullptr)
        {
            return spawnTransform;
        }
    }

    return ownerTransform;
}

void LyrielAbilityBase::faceDirection(const Vector3& direction)
{
    if (m_character == nullptr)
    {
        return;
    }

    PlayerRotation* playerRotation = m_character->getPlayerRotation();
    if (playerRotation == nullptr)
    {
        return;
    }

    Vector3 flatDirection = direction;
    flatDirection.y = 0.0f;

    if (flatDirection.LengthSquared() <= 0.0001f)
    {
        return;
    }

    flatDirection.Normalize();
    playerRotation->applyFacingFromDirection(getOwner(), flatDirection, Time::getDeltaTime());
}