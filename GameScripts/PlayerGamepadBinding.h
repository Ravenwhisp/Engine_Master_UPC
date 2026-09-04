#pragma once

#include "ScriptAPI.h"

class PlayerGamepadBinding
{
public:
    static constexpr int kMaxPlayers = 2;

    static void setKeyboard(int player);
    static void setGamepad(int player, int gamepadIndex);

    static int getGamepadDeviceIndex(int player);

private:
    struct Binding
    {
        bool isGamepad;
        int  deviceIndex;
    };

    static Binding s_bindings[kMaxPlayers];
};
