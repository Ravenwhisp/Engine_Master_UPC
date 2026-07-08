#pragma once

#include "ScriptAPI.h"

// Main-menu UI sounds (2D). Wire the menu buttons' hover/click events to these exposed
// methods from the inspector (same pattern as UIController/TestingButton). Lives on a menu
// GameObject with a SOUND_SOURCE; events come from MainMenu.bnk.
class MenuSound : public Script
{
    DECLARE_SCRIPT(MenuSound)

public:
    explicit MenuSound(GameObject* owner);

    void Start() override;

    ScriptMethodList getExposedMethods() const override;

    void PlayHover();
    void PlayClick();
    void PlayStartGame();

private:
    uint32_t postEvent(const char* eventName);

    ComponentSoundSource* m_source = nullptr;
};
