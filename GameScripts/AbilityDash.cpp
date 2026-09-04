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

    m_playerController = GameObjectAPI::findScript<PlayerController>(getOwner());
    m_playerMovement = GameObjectAPI::findScript<PlayerMovement>(getOwner());

    if (m_playerController == nullptr)
    {
        Debug::warn("AbilityDash: PlayerController not found on this GameObject.");
    }

    if (m_playerMovement == nullptr)
    {
        Debug::warn("AbilityDash: PlayerMovement not found on this GameObject.");
    }
}

void AbilityDash::Update()
{
    AbilityBase::Update();

    const float dt = Time::getDeltaTime();

    onDashUpdate(dt);

    if (m_isDashing)
    {
        updateDash(dt);
        return;
    }
}

void AbilityDash::drawGizmo()
{
}

bool AbilityDash::canStartSpecificAbility() const
{
	return canDash() && m_playerController != nullptr && m_playerMovement != nullptr && !m_isDashing;
}

void AbilityDash::startAbility()
{
    startDash();
}

bool AbilityDash::canDash() const
{
    return true;
}

void AbilityDash::onDashStarted()
{
}

void AbilityDash::startDash()
{
    m_dashTimer = 0.0f;

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

    m_dashStartPosition = TransformAPI::getGlobalPosition(getOwner()->GetTransform());

    m_isDashing = true;

    notifyAbilitySuccessfullyStarted();

    setAbilityLocked(true);
    startCooldown();

    onDashStarted();
}

void AbilityDash::updateDash(float dt)
{
    m_dashTimer += dt;
    calculateDashMovement(dt);

    if (m_dashTimer >= getDashDuration())
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
    const float dashDuration = getDashDuration();
    const float dashDistance = getDashDistance();

    if (dashDuration <= 0.0001f)
    {
        return;
    }

    float t = m_dashTimer / dashDuration;
    t = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);

    const float curveSpeed = 0.5f * PI * cos(t * PI * 0.5f);
    const float currentSpeed = (dashDistance / dashDuration) * curveSpeed;

    const Vector3 velocity = m_dashDirection * currentSpeed;

    if (m_playerMovement != nullptr)
    {
        m_playerMovement->playerDashMovement(getOwner(), velocity * dt);
    }
}

