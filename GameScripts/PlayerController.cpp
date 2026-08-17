#include "pch.h"
#include "PlayerController.h"

#include "PlayerState.h"
#include "PlayerMovement.h"
#include "PlayerRotation.h"
#include "AbilityBase.h"

#include "LyrielBasicAttack.h"
#include "DeathBasicAttack.h"
#include "LyrielChargedAttack.h"
#include "DeathChargedAttack.h"
#include "LyrielDash.h"
#include "DeathDash.h"
#include "LyrielArrowVolley.h"
#include "DeathTaunt.h"

#include <cmath>

static const float PI = 3.1415926535897931f;

IMPLEMENT_SCRIPT_FIELDS(PlayerController,
    SERIALIZED_INT(m_playerIndex, "Player Index"),
    SERIALIZED_BOOL(m_godMode, "God Mode")
)

PlayerController::PlayerController(GameObject* owner)
    : Script(owner)
{
}

void PlayerController::Start()
{
    GameObject* owner   = getOwner();

    m_playerMovement = GameObjectAPI::findScript<PlayerMovement>(owner);
    m_playerRotation = GameObjectAPI::findScript<PlayerRotation>(owner);
    m_playerState = GameObjectAPI::findScript<PlayerState>(owner);

    if (m_playerMovement == nullptr)
    {
        Debug::warn("PlayerController on '%s' could not find PlayerMovement on the same GameObject.", GameObjectAPI::getName(owner));
    }

    if (m_playerRotation == nullptr)
    {
        Debug::warn("PlayerController on '%s' could not find PlayerRotation on the same GameObject.", GameObjectAPI::getName(owner));
    }

    if (m_playerState == nullptr)
    {
        Debug::warn("PlayerController on '%s' could not find PlayerState on the same GameObject.", GameObjectAPI::getName(owner));
    }

    m_basicAttack       = findBasicAttackScript(owner);
    m_chargedAttack     = findChargedAttackScript(owner);
    m_dash              = findDashScript(owner);
    m_specialAbility    = findSpecialAbilityScript(owner);

    m_cameraTransform   = SceneAPI::getDefaultCameraGameObject() ?
        GameObjectAPI::getTransform(SceneAPI::getDefaultCameraGameObject()) :
        nullptr;

}

void PlayerController::Update()
{
    GameObject* owner = getOwner();
    if (!owner)
    {
        return;
    }

    if (m_gameplayInputLocked)
    {
        if (m_playerMovement)
        {
            m_playerMovement->setMoving(false);
        }

        return;
    }

    const float dt = Time::getDeltaTime();

    const bool downed = m_playerState && m_playerState->isDowned();
    const bool canMove = m_playerState ? m_playerState->canMove() : true;

    // While downed, no input is processed at all (revive flow handles its own input).
    if (downed)
    {
        if (m_playerMovement)
        {
            m_playerMovement->setMoving(false);
        }
        return;
    }

    if (m_playerMovement)
    {
        if (!canMove)
        {
            // Locked in an attack recovery: no movement, but ability inputs still flow
            // through so the buffer in AbilityBase can queue the next combo step.
            m_playerMovement->setMoving(false);
        }
        else
        {
            const Vector2 moveAxis = Input::getMoveAxis(m_playerIndex);

            Vector3 moveDirection = readMoveDirection(moveAxis);
            const bool isMoving = (moveDirection.x != 0.0f || moveDirection.y != 0.0f || moveDirection.z != 0.0f) && m_playerMovement->canMoveFromMultiplier();
            m_playerMovement->setMoving(isMoving);

            if (isMoving)
            {
                if (m_playerRotation)
                {
                    Vector3 horizontalDir(moveDirection.x, 0.0f, moveDirection.z);
                    if (horizontalDir.x != 0.0f || horizontalDir.z != 0.0f)
                    {
                        horizontalDir.Normalize();
                        m_playerRotation->applyFacingFromDirection(owner, horizontalDir, dt);
                    }
                }

                moveDirection.Normalize();
                m_playerMovement->playerMovement(owner, moveDirection * dt);
            }
        }
    }

    if (m_playerState && !m_playerState->canUseAbilities())
    {
        return;
    }

    if(m_basicAttack && Input::isRightShoulderJustPressed(m_playerIndex))
    {
		m_basicAttack->tryAbility();
    }

    if(m_chargedAttack && Input::isRightTriggerJustPressed(m_playerIndex))
    {
		m_chargedAttack->tryAbility();
	}

    if(m_dash && Input::isLeftShoulderJustPressed(m_playerIndex))
    {
		m_dash->tryAbility();
	}

    if (m_specialAbility && Input::isLeftTriggerJustPressed(m_playerIndex))
    {
		m_specialAbility->tryAbility();
    }

}

Vector3 PlayerController::getMoveDirection() const
{
    const Vector2 moveAxis = Input::getMoveAxis(m_playerIndex);
    return readMoveDirection(moveAxis);
}

Vector3 PlayerController::readMoveDirection(const Vector2& moveAxis) const
{
    Vector3 cameraForward = m_cameraTransform ? TransformAPI::getForward(m_cameraTransform) : Vector3(0.0f, 0.0f, 1.0f);
    Vector3 cameraRight = m_cameraTransform ? TransformAPI::getRight(m_cameraTransform) : Vector3(1.0f, 0.0f, 0.0f);
    Vector3 up = Vector3(0.0f, 1.0f, 0.0f);

    cameraForward = cameraForward - up * cameraForward.Dot(up);
    cameraRight = cameraRight - up * cameraRight.Dot(up);

    cameraForward.Normalize();
    cameraRight.Normalize();

    Vector3 move = Vector3(-moveAxis.x, 0.0f, -moveAxis.y);

    return cameraForward * move.z + cameraRight * move.x;
}

AbilityBase* PlayerController::findBasicAttackScript(GameObject* owner)
{
    AbilityBase* basicAttack = GameObjectAPI::findScript<LyrielBasicAttack>(owner);
    if (basicAttack != nullptr)
    {
        return basicAttack;
    }

    basicAttack = GameObjectAPI::findScript<DeathBasicAttack>(owner);
    if (basicAttack != nullptr)
    {
        return basicAttack;
    }

    Debug::warn("PlayerController on '%s' could not find LyrielBasicAttack or DeathBasicAttack on the same GameObject.", GameObjectAPI::getName(owner));

    return nullptr;
}

AbilityBase* PlayerController::findChargedAttackScript(GameObject* owner)
{
    AbilityBase* chargedAttack = GameObjectAPI::findScript<LyrielChargedAttack>(owner);
    if (chargedAttack != nullptr)
    {
        return chargedAttack;
    }

    chargedAttack = GameObjectAPI::findScript<DeathChargedAttack>(owner);
    if (chargedAttack != nullptr)
    {
        return chargedAttack;
    }

    Debug::warn("PlayerController on '%s' could not find LyrielChargedAttack or DeathChargedAttack on the same GameObject.", GameObjectAPI::getName(owner));

    return nullptr;
}

AbilityBase* PlayerController::findDashScript(GameObject* owner)
{
    AbilityBase* dash = GameObjectAPI::findScript<LyrielDash>(owner);
    if (dash != nullptr)
    {
        return dash;
    }

    dash = GameObjectAPI::findScript<DeathDash>(owner);
    if (dash != nullptr)
    {
        return dash;
    }

    Debug::warn("PlayerController on '%s' could not find LyrielDash or DeathDash on the same GameObject.", GameObjectAPI::getName(owner));

    return nullptr;
}

AbilityBase* PlayerController::findSpecialAbilityScript(GameObject* owner)
{
    AbilityBase* specialAbility = GameObjectAPI::findScript<LyrielArrowVolley>(owner);
    if (specialAbility != nullptr)
    {
        return specialAbility;
    }

    specialAbility = GameObjectAPI::findScript<DeathTaunt>(owner);
    if (specialAbility != nullptr)
    {
        return specialAbility;
    }

    Debug::warn("PlayerController on '%s' could not find LyrielArrowVolley or DeathTaunt on the same GameObject.", GameObjectAPI::getName(owner));

    return nullptr;
}

IMPLEMENT_SCRIPT(PlayerController)