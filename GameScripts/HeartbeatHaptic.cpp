#include "pch.h"
#include "HeartbeatHaptic.h"

#include "PlayerController.h"
#include "PlayerGamepadBinding.h"

IMPLEMENT_SCRIPT_FIELDS(HeartbeatHaptic,
    SERIALIZED_FLOAT(m_hapticIntensity, "Heartbeat Intensity", 100.0f, 0.0f, 0.01f)
)

HeartbeatHaptic::HeartbeatHaptic(GameObject* owner) : Script(owner)
{
}

void HeartbeatHaptic::Start()
{
    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatLub(1.0f, m_variant));
    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatDub(1.0f, m_variant));

    m_playerController = GameObjectAPI::findScript<PlayerController>(getOwner());
}

void HeartbeatHaptic::getTargetDeviceIndices(int outDevices[], int& outCount) const
{
    outCount = 0;

    if (m_playerController != nullptr)
    {
        const int device = PlayerGamepadBinding::getGamepadDeviceIndex(m_playerController->getPlayerIndex());
        if (device >= 0)
            outDevices[outCount++] = device;
        return;
    }

    for (int player = 0; player < PlayerGamepadBinding::kMaxPlayers; ++player)
    {
        const int device = PlayerGamepadBinding::getGamepadDeviceIndex(player);
        if (device < 0)
            continue;

        bool alreadyTargeted = false;
        for (int i = 0; i < outCount; ++i)
        {
            if (outDevices[i] == device)
            {
                alreadyTargeted = true;
                break;
            }
        }

        if (!alreadyTargeted)
            outDevices[outCount++] = device;
    }
}

void HeartbeatHaptic::playOnTargetDevices(const char* effectId, float scale)
{
    int devices[PlayerGamepadBinding::kMaxPlayers];
    int count = 0;
    getTargetDeviceIndices(devices, count);

    for (int i = 0; i < count; ++i)
        HapticAPI::playAtScale(effectId, scale, devices[i]);
}

void HeartbeatHaptic::fireLub(float t)
{
    const bool isHealth = (m_variant == HapticEffectDefinition::HeartbeatVariant::Health);

    const char* lubName = isHealth ? "HeartbeatLub_Health" : "HeartbeatLub_Separation";

    HeartbeatCycle cycle;
    if (isHealth)
        cycle = HeartbeatCycle::fromHealth(1.0f - t); // t is danger, so HP = 1 - t
    else
        cycle = HeartbeatCycle::fromSeparation(t);

    playOnTargetDevices(lubName, t * m_hapticIntensity);

    m_dubScale = t;
    m_dubTimer = cycle.interBeatSeconds;
    m_lubTimer = -1.0f;
}

void HeartbeatHaptic::Update()
{
    if (!m_active)
        return;

    const float dt = Time::getDeltaTime();
    const float t = m_currentT;

    if (m_dubTimer < 0.0f && m_lubTimer < 0.0f)
        fireLub(t);

    if (m_dubTimer >= 0.0f)
    {
        m_dubTimer -= dt;
        if (m_dubTimer < 0.0f)
        {
            const bool isHealth = (m_variant == HapticEffectDefinition::HeartbeatVariant::Health);
            const char* dubName = isHealth ? "HeartbeatDub_Health" : "HeartbeatDub_Separation";

            playOnTargetDevices(dubName, m_dubScale * m_hapticIntensity);

            if (m_dyingBeat)
            {
                m_dyingBeat = false;
                m_active = false;
                m_dubTimer = -1.0f;
                m_lubTimer = -1.0f;
                return;
            }

            HeartbeatCycle cycle;
            if (isHealth)
                cycle = HeartbeatCycle::fromHealth(1.0f - t);
            else
                cycle = HeartbeatCycle::fromSeparation(t);

            m_lubTimer = cycle.diastoleSeconds;
        }
    }

    if (m_lubTimer >= 0.0f)
    {
        m_lubTimer -= dt;
        if (m_lubTimer < 0.0f)
            fireLub(t);
    }
}

void HeartbeatHaptic::tick(float t)
{
    m_active = true;
    m_currentT = t;
}

void HeartbeatHaptic::stop()
{
    m_active = false;
    m_dubTimer = -1.0f;
    m_lubTimer = -1.0f;
    m_dubScale = 0.0f;
}

void HeartbeatHaptic::playDyingBeat(float t)
{
    m_dyingBeat = true;
    m_active = true;
    m_currentT = t;
    fireLub(t);
}

IMPLEMENT_SCRIPT(HeartbeatHaptic)