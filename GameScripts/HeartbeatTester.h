#pragma once
#include "ScriptAPI.h"
#include "HapticEffectDefinition.h"

// -----------------------------------------------------------------------
// HeartbeatTester
//
// Drop this script on any GameObject to test heartbeat haptic feedback.
// Set Danger Value (0-1) in the editor, then:
//
//   R Shoulder - fire one lub-dub at the current danger value
//   L Shoulder - cancel all
//   Y          - toggle Health / Separation mode
// -----------------------------------------------------------------------

class HeartbeatTester : public Script
{
    DECLARE_SCRIPT(HeartbeatTester)

public:
    explicit HeartbeatTester(GameObject* owner);

    void Start()  override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

public:
    int   m_playerIndex = 0;
    int   m_hapticDeviceIndex = 0;
    float m_dangerValue = 0.5f;  // 0 = safe, 1 = critical
    bool  m_useHealthMode = true;  // false = Separation

private:
    float m_dubTimer = -1.0f;  // counts down to firing the dub; -1 = inactive
    float m_dubScale = 0.0f;   // scale captured at lub-fire time
    float m_lubTimer = -1.0f;  // counts down to firing the next lub (diastole wait)

    void fireLub();
};