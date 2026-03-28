#include "pch.h"
#include "GamepadInputTest.h"

#include "SimpleMath.h"
using DirectX::SimpleMath::Vector2;

static const ScriptFieldInfo gamepadInputTestFields[] =
{
    { "Player Index", ScriptFieldType::Int, offsetof(GamepadInputTest, m_playerIndex) },
    { "Move Speed", ScriptFieldType::Float, offsetof(GamepadInputTest, m_moveSpeed), { 0.0f, 50.0f, 0.05f } }
};

IMPLEMENT_SCRIPT_FIELDS(GamepadInputTest, gamepadInputTestFields)

GamepadInputTest::GamepadInputTest(GameObject* owner)
    : Script(owner)
{
}

void GamepadInputTest::Start()
{
}

void GamepadInputTest::Update()
{
    GameObject* owner = getOwner();
    if (!owner)
    {
        return;
    }

    const bool isConnected = Input::isGamePadConnected(m_playerIndex);
    if (isConnected != m_wasConnectedLastFrame)
    {
        Debug::log("GamepadInputTest: player %d connected = %s", m_playerIndex, isConnected ? "true" : "false");
        m_wasConnectedLastFrame = isConnected;
    }

    if (!isConnected)
    {
        return;
    }

    const Vector2 stick = Input::getGamePadLeftStick(m_playerIndex);

    Vector3 movement(stick.x, 0.0f, -stick.y);
    if (movement.x != 0.0f || movement.z != 0.0f)
    {
        movement.Normalize();

        Transform* transform = GameObjectAPI::getTransform(owner);
        const Vector3 currentPosition = TransformAPI::getPosition(transform);
        const Vector3 newPosition = currentPosition + movement * m_moveSpeed * Time::getDeltaTime();
        TransformAPI::setPosition(transform, newPosition);
    }

    const bool aPressed = Input::isGamePadAPressed(m_playerIndex);
    if (aPressed && !m_wasAPressedLastFrame)
    {
        Debug::log("GamepadInputTest: player %d pressed A", m_playerIndex);
    }
    m_wasAPressedLastFrame = aPressed;
}

IMPLEMENT_SCRIPT(GamepadInputTest)