#include "pch.h"
#include "EnvironmentSound.h"

#include <string>
#include <unordered_set>

namespace
{
    constexpr const char* k_bank = "LevelCommon.bnk";

    bool isFirstReport(const char* eventName)
    {
        static std::unordered_set<std::string> reported;
        return reported.insert(eventName).second;
    }
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

    const uint32_t playingID = AudioAPI::postEvent(source, k_bank, eventName);
    if (playingID == 0)
    {
        Debug::warn("[EnvironmentSound] '%s' on '%s' was rejected (playingID=0). Bank '%s' loaded?",
            eventName, GameObjectAPI::getName(emitter), k_bank);
    }
    else if (isFirstReport(eventName))
    {
        Debug::log("[EnvironmentSound] '%s' posted on '%s' (playingID=%u).",
            eventName, GameObjectAPI::getName(emitter), playingID);
    }

    return playingID;
}

void EnvironmentSound::playGrouped(GameObject* emitter, const char* eventName, const char* groupName, uint32_t cooldownMs)
{
    if (emitter == nullptr || eventName == nullptr || groupName == nullptr)
    {
        return;
    }

    ComponentSoundSource* source = AudioAPI::getSoundSourceComponent(emitter);
    if (source == nullptr)
    {
        Debug::warn("[EnvironmentSound] '%s' has no SOUND_SOURCE for grouped event '%s'.", GameObjectAPI::getName(emitter), eventName);
        return;
    }

    float priority = 0.0f;
    GameObject* camera = SceneAPI::getDefaultCameraGameObject();
    Transform* emitterTransform = GameObjectAPI::getTransform(emitter);
    Transform* cameraTransform = camera ? GameObjectAPI::getTransform(camera) : nullptr;
    if (emitterTransform && cameraTransform)
    {
        const Vector3 emitterPosition = TransformAPI::getGlobalPosition(emitterTransform);
        const Vector3 cameraPosition = TransformAPI::getGlobalPosition(cameraTransform);
        priority = Vector3::DistanceSquared(emitterPosition, cameraPosition);
    }

    AudioAPI::queueGroupedEvent(source, k_bank, eventName, groupName, priority, cooldownMs);
}
