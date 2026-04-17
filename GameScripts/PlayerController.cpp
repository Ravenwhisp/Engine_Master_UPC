#include "pch.h"
#include "PlayerController.h"

#include <cmath>
#include <cstring>

static const float PI = 3.1415926535897931f;

static const ScriptFieldInfo playerWalkFields[] =
{
    { "Move Speed", ScriptFieldType::Float, offsetof(PlayerController, m_moveSpeed), { 0.0f, 50.0f, 0.05f } },
    { "Shift Multiplier", ScriptFieldType::Float, offsetof(PlayerController, m_shiftMultiplier), { 1.0f, 10.0f, 0.05f } },
    { "Turn Speed (deg/s)", ScriptFieldType::Float, offsetof(PlayerController, m_turnSpeedDegPerSec), { 0.0f, 2000.0f, 1.0f } },
    { "Player Index", ScriptFieldType::Int, offsetof(PlayerController, m_playerIndex) },
    { "Constrain To NavMesh", ScriptFieldType::Bool, offsetof(PlayerController, m_constrainToNavMesh) },
    { "Nav Extents", ScriptFieldType::Vec3, offsetof(PlayerController, m_navExtents) },

    // Animation triggers
    { "Start Move Trigger", ScriptFieldType::String, offsetof(PlayerController, m_startMoveTriggerName) },
    { "Stop Move Trigger", ScriptFieldType::String, offsetof(PlayerController, m_stopMoveTriggerName) },
    { "Start Run Trigger", ScriptFieldType::String, offsetof(PlayerController, m_startRunTriggerName) },
    { "Stop Run Trigger", ScriptFieldType::String, offsetof(PlayerController, m_stopRunTriggerName) },
};

IMPLEMENT_SCRIPT_FIELDS(PlayerController, playerWalkFields)

PlayerController::PlayerController(GameObject* owner)
    : Script(owner)
{
}

void PlayerController::Start()
{
    GameObject* owner = getOwner();
    m_initialRotationOffset = TransformAPI::getEulerDegrees(GameObjectAPI::getTransform(owner));
}

void PlayerController::Update()
{
    GameObject* owner = getOwner();
    if (!owner)
    {
        return;
    }

   /* if (dead)
        return;*/

    const Vector3 direction = readMoveDirection();
    const bool isMoving = !isZeroMovement(direction);
    const bool shiftHeld = Input::isKeyDown(KeyCode::LeftShift) || Input::isKeyDown(KeyCode::RightShift);
    const bool isRunning = isMoving && shiftHeld;

    updateLocomotionAnimation(owner, isMoving, isRunning);

    if (!isMoving)
    {
        return;
    }

    const float dt = Time::getDeltaTime();

    Vector3 horizontalDir(direction.x, 0.0f, direction.z);
    if (horizontalDir.x != 0.0f || horizontalDir.z != 0.0f)
    {
        horizontalDir.Normalize();
        applyFacingFromDirection(owner, -horizontalDir, dt); // Dani: positive horizontalDir makes character facing the opposite direction
    }

    Vector3 normalizedDirection = direction;
    normalizedDirection.Normalize();

    applyTranslation(owner, normalizedDirection, dt, shiftHeld);
}

void PlayerController::onAfterDeserialize()
{
    m_yawInitialized = false;
    m_currentYawDeg = 0.0f;
}

Vector3 PlayerController::readMoveDirection() const
{
    const Vector2 moveAxis = Input::getMoveAxis(m_playerIndex);

    return Vector3(-moveAxis.x, 0.0f, -moveAxis.y); // Dani: positive axis applies opposite direction from game perspective
}

void PlayerController::applyFacingFromDirection(GameObject* owner, const Vector3& direction, float dt)
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

    TransformAPI::setRotationEuler(GameObjectAPI::getTransform(owner), Vector3(m_initialRotationOffset.x, finalYaw, m_initialRotationOffset.z));
}

void PlayerController::applyTranslation(GameObject* owner, const Vector3& direction, float dt, bool shiftHeld) const
{
    Transform* transform = GameObjectAPI::getTransform(owner);
    if (!transform)
    {
        return;
    }

    float speed = m_moveSpeed;
    if (shiftHeld)
    {
        speed *= m_shiftMultiplier;
    }

    const Vector3 currentPos = TransformAPI::getPosition(transform);
    const Vector3 desiredPos = currentPos + direction * speed * dt;

    if (!m_constrainToNavMesh)
    {
        TransformAPI::setPosition(transform, desiredPos);
        return;
    }

    Vector3 constrainedPos;
    if (NavigationAPI::moveAlongSurface(currentPos, desiredPos, constrainedPos, m_navExtents))
    {
        TransformAPI::setPosition(transform, constrainedPos);
    }
}

void PlayerController::updateLocomotionAnimation(GameObject* owner, bool isMoving, bool isRunning)
{
    // No state machine authority here: only notify changes through triggers.

    if (!owner)
    {
        return;
    }

    // Enter locomotion
    if (!m_wasMoving && isMoving)
    {
        sendAnimationTrigger(owner, isRunning ? m_startRunTriggerName : m_startMoveTriggerName);
    }
    // Exit locomotion
    else if (m_wasMoving && !isMoving)
    {
        sendAnimationTrigger(owner, m_wasRunning ? m_stopRunTriggerName : m_stopMoveTriggerName);
    }
    // Change locomotion mode while moving
    else if (m_wasMoving && isMoving)
    {
        if (!m_wasRunning && isRunning)
        {
            sendAnimationTrigger(owner, m_startRunTriggerName);
        }
        else if (m_wasRunning && !isRunning)
        {
            sendAnimationTrigger(owner, m_stopRunTriggerName);
        }
    }

    m_wasMoving = isMoving;
    m_wasRunning = isRunning;
}

void PlayerController::sendAnimationTrigger(GameObject* owner, const std::string& triggerName) const
{
    if (!owner || triggerName.empty())
    {
        return;
    }

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(owner);
    if (!animation || !AnimationAPI::hasStateMachine(animation))
    {
        return;
    }

    AnimationAPI::sendTrigger(animation, triggerName.c_str());
}

float PlayerController::wrapAngleDegrees(float angle)
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


bool PlayerController::isZeroMovement(const Vector3& direction)
{
    return direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f;
}

float PlayerController::moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta)
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

IMPLEMENT_SCRIPT(PlayerController)