#include "pch.h"
#include "PlayerWalk.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleTime.h"
#include "GameObject.h"
#include "Transform.h"

static const float PI = 3.1415926535897931f;

PlayerWalk::PlayerWalk(GameObject* owner)
    : Script(owner)
{
}

void PlayerWalk::Start()
{
    inputModule = app->getModuleInput();

    Transform* transform = getOwner()->GetTransform();
    m_initialRotationOffset = transform->getEulerDegrees();
}

void PlayerWalk::Update()
{
    if (!getOwner())
    {
        return;
    }

    Transform* transform = getOwner()->GetTransform();
    if (!transform)
    {
        return;
    }

    Vector3 direction = readMoveDirection(inputModule);

    if (direction == Vector3::Zero)
    {
        return;
    }

    const float dt = getDeltaSecondsFromTimer();
    bool shiftHeld = checkShiftHeld(inputModule);

    Vector3 horizontalDir(direction.x, 0.0f, direction.z);
    if (horizontalDir != Vector3::Zero)
    {
        horizontalDir.Normalize();
        applyFacingFromDirection(transform, horizontalDir, dt);
    }

    direction.Normalize();
    applyTranslation(transform, direction, dt, shiftHeld);
}

std::unique_ptr<Script> PlayerWalk::Create(GameObject* owner)
{
    return std::make_unique<PlayerWalk>(owner);
}

float PlayerWalk::getDeltaSecondsFromTimer() const
{
    return app->getModuleTime()->deltaTime();
}

Vector3 PlayerWalk::readMoveDirection(ModuleInput* inputModule) const
{
    Vector3 direction(0, 0, 0);

    if (inputModule->isKeyDown(m_keyUp))
    {
        direction.z -= 1.0f;
    }
    if (inputModule->isKeyDown(m_keyDown))
    {
        direction.z += 1.0f;
    }
    if (inputModule->isKeyDown(m_keyLeft))
    {
        direction.x -= 1.0f;
    }
    if (inputModule->isKeyDown(m_keyRight))
    {
        direction.x += 1.0f;
    }
    if (inputModule->isKeyDown(m_keyAscend))
    {
        direction.y += 1.0f;
    }
    if (inputModule->isKeyDown(m_keyDescend))
    {
        direction.y -= 1.0f;
    }

    return direction;
}

void PlayerWalk::applyFacingFromDirection(Transform* transform, const Vector3& direction, float dt)
{
    const float yawRad = std::atan2(-direction.x, -direction.z);
    const float targetYawDeg = yawRad * (180.0f / PI);

    if (!m_yawInitialized)
    {
        m_currentYawDeg = 0.0f;
        m_yawInitialized = true;
    }

    const float maxStep = m_turnSpeedDegPerSec * dt;
    m_currentYawDeg = moveTowardsAngleDegrees(m_currentYawDeg, targetYawDeg, maxStep);

    const float finalYaw = wrapAngleDegrees(m_initialRotationOffset.y + m_currentYawDeg);

    transform->setRotationEuler(Vector3(m_initialRotationOffset.x, finalYaw, m_initialRotationOffset.z));
}

void PlayerWalk::applyTranslation(Transform* transform, const Vector3& direction, float dt, bool shiftHeld) const
{
    float speed = m_moveSpeed;

    if (shiftHeld)
    {
        speed *= m_shiftMultiplier;
    }

    float step = speed * dt;

    Vector3 pos = transform->getPosition();
    pos += direction * step;

    transform->setPosition(pos);
}

bool PlayerWalk::checkShiftHeld(ModuleInput* inputModule) const
{
    return inputModule->isKeyDown(Keyboard::Keys::LeftShift) ||
        inputModule->isKeyDown(Keyboard::Keys::RightShift);
}

float PlayerWalk::wrapAngleDegrees(float angle)
{
    while (angle > 180.0f)
    {
        angle -= 360.0f;
    }
    while (angle < -180.0f)
    {
        angle += 360.0f;
    }
    return angle;
}

float PlayerWalk::moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta)
{
    float delta = wrapAngleDegrees(targetYawAngle - currentYawAngle);

    if (delta > maxDelta)
    {
        delta = maxDelta;
    }

    if (delta < -maxDelta)
    {
        delta = -maxDelta;
    }

    return currentYawAngle + delta;
}