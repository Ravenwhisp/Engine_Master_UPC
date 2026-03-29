#include "pch.h"
#include "LocalMultiplayerSetup.h"

static const char* setupModeNames[] =
{
    "Keyboard + Gamepad",
    "Two Gamepads"
};

static const ScriptFieldInfo localMultiplayerSetupFields[] =
{
    { "Setup Mode", ScriptFieldType::EnumInt, offsetof(LocalMultiplayerSetup, m_setupMode), {}, { setupModeNames, 2 } }
};

IMPLEMENT_SCRIPT_FIELDS(LocalMultiplayerSetup, localMultiplayerSetupFields)

LocalMultiplayerSetup::LocalMultiplayerSetup(GameObject* owner)
    : Script(owner)
{
}

void LocalMultiplayerSetup::Start()
{
    chooseConfiguration();
}

void LocalMultiplayerSetup::setKeyboardGamepad()
{
    setMode(0);
}

void LocalMultiplayerSetup::setTwoGamepad()
{
    setMode(1);
}

void LocalMultiplayerSetup::setMode(int mode)
{
    m_setupMode = mode;
    chooseConfiguration();
}

void LocalMultiplayerSetup::chooseConfiguration()
{
    switch (m_setupMode)
    {
    case 0:
        Input::setPlayerKeyboard(0);
        Input::setPlayerGamepad(1, 0);
        break;

    case 1:
        Input::setPlayerGamepad(0, 0);
        Input::setPlayerGamepad(1, 1);
        break;

    default:
        Input::setPlayerKeyboard(0);
        Input::setPlayerGamepad(1, 0);
        break;
    }
}

IMPLEMENT_SCRIPT(LocalMultiplayerSetup)