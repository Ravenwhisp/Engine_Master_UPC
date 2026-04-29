#include "pch.h"
#include "HapticTester.h"

IMPLEMENT_SCRIPT_FIELDS(HapticTester,
    SERIALIZED_INT(m_playerIndex, "Player Index"),
    SERIALIZED_STRING(m_namedEffectId, "Named Effect Id"),
)

HapticTester::HapticTester(GameObject* owner)
    : Script(owner)
{
}

void HapticTester::Start()
{
    m_activeHandle = 0;
    Debug::log("[HapticTester] Ready on player %d. Button map:", m_playerIndex);
    Debug::log("  A – playEffect('%s')", m_namedEffectId.c_str());
    Debug::log("  R Shoulder– cancelEffect(activeHandle)");
    Debug::log("  L Trigger – cancelAll");
    Debug::log("  R Trigger – log active effects");
}

void HapticTester::Update()
{
    const int p = m_playerIndex;

    if (Input::isFaceButtonBottomJustPressed(p))
    {
        m_activeHandle = HapticAPI::playEffect(m_namedEffectId.c_str(), m_hapticDeviceIndex);
        Debug::log("[HapticTester] playEffect('%s') -> handle %u", m_namedEffectId.c_str(), m_activeHandle);
    }

    if (Input::isRightShoulderJustPressed(p))
    {
        if (m_activeHandle != 0)
        {
            Debug::log("[HapticTester] cancelEffect(handle=%u)", m_activeHandle);
            HapticAPI::cancelEffect(m_activeHandle, m_hapticDeviceIndex);
            m_activeHandle = 0;
        }
        else
        {
            Debug::log("[HapticTester] cancelEffect: no active handle to cancel.");
        }
    }

    if (Input::isLeftTriggerJustPressed(p))
    {
        Debug::log("[HapticTester] cancelAll (player %d)", p);
        HapticAPI::cancelAll(p);
        m_activeHandle = 0;
    }

    if (Input::isRightTriggerJustPressed(p))
    {
        Debug::log("[HapticTester] isPlaying = %s", HapticAPI::isPlaying(p) ? "true" : "false");
    }
}

IMPLEMENT_SCRIPT(HapticTester)