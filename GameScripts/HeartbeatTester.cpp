#include "pch.h"
#include "HeartbeatTester.h"

IMPLEMENT_SCRIPT_FIELDS(HeartbeatTester,
    SERIALIZED_INT(m_playerIndex, "Player Index"),
    SERIALIZED_INT(m_hapticDeviceIndex, "Haptic Device Index"),
    SERIALIZED_FLOAT(m_dangerValue, "Danger Value", 0.0f, 1.0f, 0.01f),
    SERIALIZED_BOOL(m_useHealthMode, "Health Mode (false = Separation)")
)

HeartbeatTester::HeartbeatTester(GameObject* owner) : Script(owner) {}

// ---------------------------------------------------------------------------
void HeartbeatTester::Start()
{
    const auto healthVariant = HapticEffectDefinition::HeartbeatVariant::Health;
    const auto sepVariant = HapticEffectDefinition::HeartbeatVariant::Separation;

    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatLub(1.0f, healthVariant));
    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatDub(1.0f, healthVariant));
    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatLub(1.0f, sepVariant));
    HapticAPI::registerEffect(HapticEffectDefinition::makeHeartbeatDub(1.0f, sepVariant));

    Debug::log("[HeartbeatTester] Starting heartbeat loop on player %d.", m_playerIndex);
    Debug::log("  L Shoulder - stop/start");
    Debug::log("  Y          - toggle Health / Separation mode");

    fireLub();  // kick off the first beat immediately
}

// ---------------------------------------------------------------------------
void HeartbeatTester::fireLub()
{
    const HeartbeatCycle cycle = m_useHealthMode
        ? HeartbeatCycle::fromHealth(1.0f - m_dangerValue)
        : HeartbeatCycle::fromSeparation(m_dangerValue);

    const char* lubId = m_useHealthMode
        ? "HeartbeatLub_Health"
        : "HeartbeatLub_Separation";

    HapticAPI::playAtScale(lubId, m_dangerValue, m_hapticDeviceIndex);

    m_dubScale = m_dangerValue;

    // Use heartbeat-defined spacing between lub and dub
    m_dubTimer = cycle.interBeatSeconds;

    m_lubTimer = -1.0f;
}

// ---------------------------------------------------------------------------
void HeartbeatTester::Update()
{
    const int p = m_playerIndex;
    const float dt = Time::getDeltaTime();

    // --- Dub countdown ---
    if (m_dubTimer >= 0.0f)
    {
        m_dubTimer -= dt;
        if (m_dubTimer < 0.0f)
        {
            const char* dubId = m_useHealthMode ? "HeartbeatDub_Health" : "HeartbeatDub_Separation";
            HapticAPI::playAtScale(dubId, m_dubScale, m_hapticDeviceIndex);

            // Use HeartbeatCycle to get the correct diastole length for current danger
            const HeartbeatCycle cycle = m_useHealthMode
                ? HeartbeatCycle::fromHealth(1.0f - m_dangerValue)
                : HeartbeatCycle::fromSeparation(m_dangerValue);

            m_lubTimer = cycle.diastoleSeconds;  // wait the diastole, then fire next lub
        }
    }

    // --- Next lub countdown (diastole wait) ---
    if (m_lubTimer >= 0.0f)
    {
        m_lubTimer -= dt;
        if (m_lubTimer < 0.0f)
            fireLub();
    }

    // --- Controls ---
    if (Input::isLeftShoulderJustPressed(p))
    {
        if (m_dubTimer >= 0.0f || m_lubTimer >= 0.0f)
        {
            // Stop
            HapticAPI::cancelAll(m_hapticDeviceIndex);
            m_dubTimer = -1.0f;
            m_lubTimer = -1.0f;
            Debug::log("[HeartbeatTester] Stopped.");
        }
        else
        {
            // Start
            Debug::log("[HeartbeatTester] Started.");
            fireLub();
        }
    }

    if (Input::isFaceButtonTopJustPressed(p))   // Y
    {
        m_useHealthMode = !m_useHealthMode;
        Debug::log("[HeartbeatTester] Mode -> %s", m_useHealthMode ? "Health" : "Separation");
    }
}

IMPLEMENT_SCRIPT(HeartbeatTester)