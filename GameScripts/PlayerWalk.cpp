#include "pch.h"
#include "PlayerWalk.h"
#include "ScriptAPI.h"

static const float PI = 3.1415926535897931f;

static const char* controlSchemeNames[] =
{
    "WASD",
    "IJKL"
};

static const ScriptFieldInfo playerWalkFields[] =
{
    { "Move Speed", ScriptFieldType::Float, offsetof(PlayerWalk, m_moveSpeed), { 0.0f, 50.0f, 0.05f } },
    { "Shift Multiplier", ScriptFieldType::Float, offsetof(PlayerWalk, m_shiftMultiplier), { 1.0f, 10.0f, 0.05f } },
    { "Control Scheme", ScriptFieldType::EnumInt, offsetof(PlayerWalk, m_controlScheme), {}, {controlSchemeNames, 2} }
};

IMPLEMENT_SCRIPT_FIELDS(PlayerWalk, playerWalkFields)

PlayerWalk::PlayerWalk(GameObject* owner)
    : Script(owner)
{
}

void PlayerWalk::Start()
{
    GameObject* owner = getOwner();
    m_initialRotationOffset = TransformAPI::getEulerDegrees(GameObjectAPI::getTransform(owner));

    applyControlScheme();
}

void PlayerWalk::Update()
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
    bool shiftHeld = Input::isKeyDown((int)Keyboard::Keys::LeftShift) || Input::isKeyDown((int)Keyboard::Keys::RightShift);

    Vector3 horizontalDir(direction.x, 0.0f, direction.z);
    if (horizontalDir.x != 0.0f || horizontalDir.z != 0.0f)
    {
        horizontalDir.Normalize();
        applyFacingFromDirection(owner, horizontalDir, dt);
    }

    direction.Normalize();
    applyTranslation(owner, direction, dt, shiftHeld);
}

void PlayerWalk::onFieldEdited(const ScriptFieldInfo& field)
{
    if (std::strcmp(field.name, "Control Scheme") == 0)
    {
        applyControlScheme();
    }
}

void PlayerWalk::onAfterDeserialize()
{
    applyControlScheme();
}

Vector3 PlayerWalk::readMoveDirection() const
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
    if (Input::isKeyDown((int)m_keyAscend))
    {
        direction.y += 1.0f;
    }
    if (Input::isKeyDown((int)m_keyDescend))
    {
        direction.y -= 1.0f;
    }

    return direction;
}

void PlayerWalk::applyFacingFromDirection(GameObject* owner, const Vector3& direction, float dt)
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

void PlayerWalk::applyTranslation(GameObject* owner, const Vector3& direction, float dt, bool shiftHeld) const
{
    float speed = m_moveSpeed;
    if (shiftHeld)
    {
        speed *= m_shiftMultiplier;
    }

    TransformAPI::translate(GameObjectAPI::getTransform(owner), direction * speed * dt);
}

void PlayerWalk::applyControlScheme()
{
    switch (m_controlScheme)
    {
    case ControlScheme::IJKL:
        m_keyUp = Keyboard::Keys::I;
        m_keyLeft = Keyboard::Keys::J;
        m_keyDown = Keyboard::Keys::K;
        m_keyRight = Keyboard::Keys::L;
        m_keyAscend = Keyboard::Keys::O;
        m_keyDescend = Keyboard::Keys::U;
        break;

    case ControlScheme::WASD:
    default:
        m_keyUp = Keyboard::Keys::W;
        m_keyLeft = Keyboard::Keys::A;
        m_keyDown = Keyboard::Keys::S;
        m_keyRight = Keyboard::Keys::D;
        m_keyAscend = Keyboard::Keys::E;
        m_keyDescend = Keyboard::Keys::Q;
        break;
    }
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

IMPLEMENT_SCRIPT(PlayerWalk)