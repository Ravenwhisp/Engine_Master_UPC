#include "pch.h"
#include "PlayerController.h"

#include <cmath>

static const float PI = 3.1415926535897931f;

static const char* controlSchemeNames[] =
{
    "WASD",
    "IJKL"
};

static const ScriptFieldInfo playerWalkFields[] =
{
    { "Move Speed", ScriptFieldType::Float, offsetof(PlayerController, m_moveSpeed), { 0.0f, 50.0f, 0.05f } },
    { "Shift Multiplier", ScriptFieldType::Float, offsetof(PlayerController, m_shiftMultiplier), { 1.0f, 10.0f, 0.05f } },
    { "Turn Speed (deg/s)", ScriptFieldType::Float, offsetof(PlayerController, m_turnSpeedDegPerSec), { 0.0f, 2000.0f, 1.0f } },
    { "Control Scheme", ScriptFieldType::EnumInt, offsetof(PlayerController, m_controlScheme), {}, {controlSchemeNames, 2} },
    { "Constrain To NavMesh", ScriptFieldType::Bool, offsetof(PlayerController, m_constrainToNavMesh) },
    { "Nav Extents", ScriptFieldType::Vec3, offsetof(PlayerController, m_navExtents) }
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

    applyControlScheme();
}

void PlayerController::Update()
{
    GameObject* owner = getOwner();
    if (!owner)
    {
        return;
    }

    Vector3 direction = readMoveDirection();

    if (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f)
    {
        return;
    }

    const float dt = Time::getDeltaTime();
    bool shiftHeld = Input::isKeyDown((int)DirectX::Keyboard::Keys::LeftShift) || Input::isKeyDown((int)DirectX::Keyboard::Keys::RightShift);

    Vector3 horizontalDir(direction.x, 0.0f, direction.z);
    if (horizontalDir.x != 0.0f || horizontalDir.z != 0.0f)
    {
        horizontalDir.Normalize();
        applyFacingFromDirection(owner, horizontalDir, dt);
    }

    direction.Normalize();
    applyTranslation(owner, direction, dt, shiftHeld);
}

void PlayerController::onFieldEdited(const ScriptFieldInfo& field)
{
    if (std::strcmp(field.name, "Control Scheme") == 0)
    {
        applyControlScheme();
    }
}

void PlayerController::onAfterDeserialize()
{
    applyControlScheme();
    m_yawInitialized = false;
    m_currentYawDeg = 0.0f;
}

Vector3 PlayerController::readMoveDirection() const
{
    Vector3 direction(0, 0, 0);

    if (Input::isKeyDown((int)m_keyUp))
    {
        direction.z -= 1.0f;
    }
    if (Input::isKeyDown((int)m_keyDown))
    {
        direction.z += 1.0f;
    }
    if (Input::isKeyDown((int)m_keyLeft))
    {
        direction.x -= 1.0f;
    }
    if (Input::isKeyDown((int)m_keyRight))
    {
        direction.x += 1.0f;
    }

    return direction;
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

void PlayerController::applyControlScheme()
{
    switch (m_controlScheme)
    {
    case ControlScheme::IJKL:
        m_keyUp = Keyboard::Keys::I;
        m_keyLeft = Keyboard::Keys::J;
        m_keyDown = Keyboard::Keys::K;
        m_keyRight = Keyboard::Keys::L;
        break;

    case ControlScheme::WASD:
    default:
        m_keyUp = Keyboard::Keys::W;
        m_keyLeft = Keyboard::Keys::A;
        m_keyDown = Keyboard::Keys::S;
        m_keyRight = Keyboard::Keys::D;
        break;
    }
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