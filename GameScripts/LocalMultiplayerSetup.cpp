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