#include "pch.h"
#include "PlayerGamepadBinding.h"

PlayerGamepadBinding::Binding PlayerGamepadBinding::s_bindings[kMaxPlayers] =
{
    { false, 0 },
    { true,  0 },
};

void PlayerGamepadBinding::setKeyboard(int player)
{
    Input::setPlayerKeyboard(player);

    if (player >= 0 && player < kMaxPlayers)
        s_bindings[player] = { false, 0 };
}

void PlayerGamepadBinding::setGamepad(int player, int gamepadIndex)
{
    Input::setPlayerGamepad(player, gamepadIndex);

    if (player >= 0 && player < kMaxPlayers)
        s_bindings[player] = { true, gamepadIndex };
}

int PlayerGamepadBinding::getGamepadDeviceIndex(int player)
{
    if (player < 0 || player >= kMaxPlayers)
        return -1;

    const Binding& binding = s_bindings[player];
    return binding.isGamepad ? binding.deviceIndex : -1;
}
