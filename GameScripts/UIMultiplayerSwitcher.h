#pragma once

#include "ScriptAPI.h"

class UIMultiplayerSwitcher : public Script
{
    DECLARE_SCRIPT(UIMultiplayerSwitcher)

public:
    explicit UIMultiplayerSwitcher(GameObject* owner);

    void Start() override;
    void Update() override;

    ComponentRef<Transform> keyboardGamepadImage;
    ComponentRef<Transform> twoGamepadImage;
};