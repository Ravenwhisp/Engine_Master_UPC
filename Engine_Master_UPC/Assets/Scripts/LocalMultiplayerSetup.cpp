#include "pch.h"
#include "LocalMultiplayerSetup.h"

static const char* setupModeNames[] =
{
    "Keyboard + Gamepad",
    "Two Gamepads"
};

IMPLEMENT_SCRIPT_FIELDS(LocalMultiplayerSetup,
    SERIALIZED_ENUM_INT(m_setupMode, "Setup Mode", setupModeNames, 2),
    SERIALIZED_COMPONENT_REF(keyboardGamepadButton, "Keyboard + Gamepad Button", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(twoGamepadButton, "Two Gamepads Button", ComponentType::TRANSFORM)
)

static const ScriptMethodInfo UIControllerMethods[] =
{
    {"Set Keyboard + Gamepad", [](Script* s) { static_cast<LocalMultiplayerSetup*>(s)->setKeyboardGamepad(); } },
    { "Set Two Gamepads", [](Script* s) { static_cast<LocalMultiplayerSetup*>(s)->setTwoGamepad(); } }
};

LocalMultiplayerSetup::LocalMultiplayerSetup(GameObject* owner)
    : Script(owner)
{
}

void LocalMultiplayerSetup::Start()
{
    chooseConfiguration();
}

ScriptMethodList LocalMultiplayerSetup::getExposedMethods() const
{
    return { UIControllerMethods, sizeof(UIControllerMethods) / sizeof(ScriptMethodInfo) };
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
    Transform* kbComp = keyboardGamepadButton.getReferencedComponent();
    Transform* gpComp = twoGamepadButton.getReferencedComponent();

    switch (m_setupMode)
    {
    case 0:
        Input::setPlayerKeyboard(0);
        Input::setPlayerGamepad(1, 0);
        if (kbComp && gpComp)
        {
            GameObjectAPI::setActive(kbComp->getOwner(), true);
            GameObjectAPI::setActive(gpComp->getOwner(), false);
        }
        break;

    case 1:
        Input::setPlayerGamepad(0, 0);
        Input::setPlayerGamepad(1, 1);
        if (kbComp && gpComp)
        {
            GameObjectAPI::setActive(kbComp->getOwner(), false);
            GameObjectAPI::setActive(gpComp->getOwner(), true);
        }
        break;

    default:
        Input::setPlayerKeyboard(0);
        Input::setPlayerGamepad(1, 0);
        if (kbComp && gpComp)
        {
            GameObjectAPI::setActive(kbComp->getOwner(), true);
            GameObjectAPI::setActive(gpComp->getOwner(), false);
        }
        break;
    }
}

IMPLEMENT_SCRIPT(LocalMultiplayerSetup)