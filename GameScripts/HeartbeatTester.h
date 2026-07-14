#pragma once
#include "ScriptAPI.h"
#include "HapticEffectDefinition.h"

class HeartbeatTester : public Script
{
    DECLARE_SCRIPT(HeartbeatTester)

public:
    explicit HeartbeatTester(GameObject* owner);

    void Start()  override;
    void Update() override;

    FieldList getExposedFields() const override;

public:
    int   m_playerIndex = 0;
    int   m_hapticDeviceIndex = 0;
    float m_dangerValue = 0.5f;  // 0 = safe, 1 = critical
    bool  m_useHealthMode = true; 

private:
    float m_dubTimer = -1.0f;  
    float m_dubScale = 0.0f;   
    float m_lubTimer = -1.0f; 

    void fireLub();
};