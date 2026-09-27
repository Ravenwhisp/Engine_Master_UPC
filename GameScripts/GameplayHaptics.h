#pragma once

#include "ScriptAPI.h"
#include "PlayerGamepadBinding.h"

#include <algorithm>

// Refreshes a short rumble at a fixed cadence. Stopping refreshes lets the
// final pulse expire without cancelling other gameplay or health feedback.
class GameplayHapticRumble
{
public:
    void update(int playerIndex, float leftMotor, float rightMotor)
    {
        m_refreshTimer -= Time::getDeltaTime();

        if (m_refreshTimer > 0.0f)
        {
            return;
        }

        const int deviceIndex = PlayerGamepadBinding::getGamepadDeviceIndex(playerIndex);
        if (deviceIndex >= 0)
        {
            HapticAPI::submitRumble(
                std::clamp(leftMotor, 0.0f, 1.0f),
                std::clamp(rightMotor, 0.0f, 1.0f),
                k_pulseDuration,
                deviceIndex);
        }

        m_refreshTimer = k_refreshInterval;
    }

    void stop()
    {
        m_refreshTimer = 0.0f;
    }

    static void playImpact(int playerIndex, float intensity, float duration)
    {
        const int deviceIndex = PlayerGamepadBinding::getGamepadDeviceIndex(playerIndex);
        if (deviceIndex >= 0)
        {
            HapticAPI::submitImpact(std::clamp(intensity, 0.0f, 1.0f), duration, deviceIndex);
        }
    }

private:
    static constexpr float k_refreshInterval = 0.10f;
    static constexpr float k_pulseDuration = 0.15f;

    float m_refreshTimer = 0.0f;
};
