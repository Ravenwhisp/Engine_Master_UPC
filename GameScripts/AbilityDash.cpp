#include "pch.h"
#include "AbilityDash.h"

#include "CharacterBase.h"
#include "PlayerController.h"
#include "PlayerMovement.h"

#define PI 3.1415926535897931f

AbilityDash::AbilityDash(GameObject* owner)
    : AbilityBase(owner)
{
}

void AbilityDash::Start()
{
    AbilityBase::Start();

    m_playerController = findControllerScript(getOwner());
    m_playerMovement = findMovementScript(getOwner());

    if (m_character == nullptr)
    {
        Debug::warn("AbilityDash: CharacterBase not found on this GameObject.");
    }
}

void AbilityDash::Update()
{
    AbilityBase::Update();

    if (m_character == nullptr)
    {
        return;
    }

    const float dt = Time::getDeltaTime();

    onDashUpdate(dt);

    if (m_isDashing)
    {
        updateDash(dt);
        return;
    }

    if (Input::isLeftShoulderJustPressed(getPlayerIndex()))
    {
        tryStartDash();
    }
}

void AbilityDash::drawGizmo()
{
}

bool AbilityDash::canDash() const
{
    return true;
}

void AbilityDash::onDashStarted()
{
}

void AbilityDash::tryStartDash()
{
    if (!canStartAbility())
    {
        return;
    }

    if (!canDash())
    {
        return;
    }

    if (m_playerController == nullptr || m_playerMovement == nullptr)
    {
        return;
    }

    m_dashTimer = 0.0f;
    m_isDashing = true;

    setAbilityLocked(true);
    m_cooldownTimer = m_cooldown;

    Vector3 moveDirection = m_playerController->getMoveDirection();

    if (moveDirection.LengthSquared() <= 0.0001f)
    {
        Transform* ownerTransform = GameObjectAPI::getTransform(getOwner());
        if (ownerTransform != nullptr)
        {
            m_dashDirection = TransformAPI::getForward(ownerTransform);
        }
        else
        {
            m_dashDirection = Vector3::Zero;
        }
    }
    else
    {
        m_dashDirection = moveDirection;
    }

    m_dashDirection.y = 0.0f;
    if (m_dashDirection.LengthSquared() > 0.0001f)
    {
        m_dashDirection.Normalize();
    }

    onDashStarted();
}

void AbilityDash::updateDash(float dt)
{
    m_dashTimer += dt;
    calculateDashMovement(dt);

    if (m_dashTimer >= m_dashDuration)
    {
        stopDash();
    }
}

void AbilityDash::stopDash()
{
    m_isDashing = false;
    onDashEnded();
    m_dashTimer = 0.0f;
    m_dashDirection = Vector3::Zero;

    setAbilityLocked(false);
}

void AbilityDash::calculateDashMovement(float dt)
{
    if (m_dashDuration <= 0.0001f)
    {
        return;
    }

    float t = m_dashTimer / m_dashDuration;
    t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);

    const float curveSpeed = 0.5f * PI * cos(t * PI * 0.5f);
    const float currentSpeed = (m_dashDistance / m_dashDuration) * curveSpeed;

    const Vector3 velocity = m_dashDirection * currentSpeed;

    if (m_playerMovement != nullptr)
    {
        m_playerMovement->playerDashMovement(getOwner(), velocity * dt);
    }
}

PlayerController* AbilityDash::findControllerScript(GameObject* owner) const
{
    Script* script = GameObjectAPI::getScript(owner, "PlayerController");
    if (script == nullptr)
    {
        Debug::warn("AbilityDash: PlayerController not found on this GameObject.");
        return nullptr;
    }

    return static_cast<PlayerController*>(script);
}

PlayerMovement* AbilityDash::findMovementScript(GameObject* owner) const
{
    Script* script = GameObjectAPI::getScript(owner, "PlayerMovement");
    if (script == nullptr)
    {
        Debug::warn("AbilityDash: PlayerMovement not found on this GameObject.");
        return nullptr;
    }

    return static_cast<PlayerMovement*>(script);
}

