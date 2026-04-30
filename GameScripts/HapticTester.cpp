#include "pch.h"
#include "HapticTester.h"

IMPLEMENT_SCRIPT_FIELDS(HapticTester,
    SERIALIZED_INT(m_playerIndex, "Player Index"),
    SERIALIZED_STRING(m_namedEffectId, "Named Effect Id"),
    )

    HapticTester::HapticTester(GameObject* owner) : Script(owner) {}

std::vector<HapticEffectDefinition> HapticTester::defineEffects()
{
    return {};
}

void HapticTester::Start()
{
    m_activeHandle = 0;

    for (const HapticEffectDefinition& def : defineEffects())
    {
        HapticAPI::registerEffect(def);    
        Debug::log("[HapticTester] Registered effect '%s'.", def.id.c_str());
    }

    Debug::log("[HapticTester] Ready on player %d.", m_playerIndex);
    Debug::log("  A - playEffect('%s')", m_namedEffectId.c_str());
    Debug::log("  R Shoulder - cancelEffect(activeHandle)");
    Debug::log("  L Trigger  - cancelAll");
    Debug::log("  R Trigger  - isPlaying");

    registerTestEffects();
}

void HapticTester::Update()
{
    const int p = m_playerIndex;

    if (Input::isFaceButtonBottomJustPressed(p))
    {
        m_activeHandle = HapticAPI::playEffect(m_namedEffectId.c_str(), m_hapticDeviceIndex);
        Debug::log("[HapticTester] playEffect('%s') -> handle %u",
            m_namedEffectId.c_str(), m_activeHandle);
    }

    if (Input::isRightShoulderJustPressed(p))
    {
        if (m_activeHandle != 0)
        {
            HapticAPI::cancelEffect(m_activeHandle, m_hapticDeviceIndex);
            Debug::log("[HapticTester] cancelEffect(handle=%u)", m_activeHandle);
            m_activeHandle = 0;
        }
    }

    if (Input::isLeftTriggerJustPressed(p))
    {
        HapticAPI::cancelAll(p);
        Debug::log("[HapticTester] cancelAll (player %d)", p);
        m_activeHandle = 0;
    }

    if (Input::isRightTriggerJustPressed(p))
    {
        Debug::log("[HapticTester] isPlaying = %s",
            HapticAPI::isPlaying(p) ? "true" : "false");
    }
}

void HapticTester::registerTestEffects()
{
    HapticEffectDefinition footsteps;
    footsteps.id = "Footsteps";
    footsteps.durationSeconds = 0.06f;
    footsteps.attackSeconds = 0.005f;
    footsteps.curve = HapticCurve::Punch;
    footsteps.priority = HapticPriority::Low;
    footsteps.peak.leftMotor = 0.25f;
    footsteps.peak.rightMotor = 0.1f;

    HapticAPI::registerEffect(footsteps);
    HapticAPI::saveToJSON("Assets/Haptics/game_effects.json");

}


IMPLEMENT_SCRIPT(HapticTester)