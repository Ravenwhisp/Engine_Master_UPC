#include "pch.h"
#include "MenuSound.h"

namespace
{
    constexpr const char* k_bank = "MainMenu.bnk";

    constexpr const char* k_hover     = "Play_Menu_Hover";
    constexpr const char* k_click     = "Play_Menu_Click";
    constexpr const char* k_startGame = "Play_Menu_StartGame";
}

MenuSound::MenuSound(GameObject* owner)
    : Script(owner)
{
}

void MenuSound::Start()
{
    m_source = AudioAPI::getSoundSourceComponent(getOwner());
    if (m_source == nullptr)
    {
        Debug::error("[MenuSound] No SOUND_SOURCE component on '%s'.",
                     GameObjectAPI::getName(getOwner()));
    }
}

uint32_t MenuSound::postEvent(const char* eventName)
{
    if (m_source == nullptr)
    {
        return 0;
    }
    return AudioAPI::postEvent(m_source, k_bank, eventName);
}

void MenuSound::PlayHover()     { postEvent(k_hover); }
void MenuSound::PlayClick()     { postEvent(k_click); }
void MenuSound::PlayStartGame() { postEvent(k_startGame); }

static const ScriptMethodInfo MenuSoundMethods[] =
{
    { "PlayHover",     [](Script* s) { static_cast<MenuSound*>(s)->PlayHover(); } },
    { "PlayClick",     [](Script* s) { static_cast<MenuSound*>(s)->PlayClick(); } },
    { "PlayStartGame", [](Script* s) { static_cast<MenuSound*>(s)->PlayStartGame(); } }
};

ScriptMethodList MenuSound::getExposedMethods() const
{
    return { MenuSoundMethods, sizeof(MenuSoundMethods) / sizeof(ScriptMethodInfo) };
}

IMPLEMENT_SCRIPT(MenuSound)
