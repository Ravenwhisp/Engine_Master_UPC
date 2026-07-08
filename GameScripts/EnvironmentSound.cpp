#include "pch.h"
#include "EnvironmentSound.h"

namespace
{
    constexpr const char* k_bank = "LevelCommon.bnk";
}

uint32_t EnvironmentSound::play(GameObject* emitter, const char* eventName)
{
    if (emitter == nullptr || eventName == nullptr)
    {
        return 0;
    }

    ComponentSoundSource* source = AudioAPI::getSoundSourceComponent(emitter);
    if (source == nullptr)
    {
        Debug::warn("[EnvironmentSound] '%s' has no SOUND_SOURCE for event '%s'.",
                    GameObjectAPI::getName(emitter), eventName);
        return 0;
    }

    return AudioAPI::postEvent(source, k_bank, eventName);
}
