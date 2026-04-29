#pragma once

#include "ScriptAPI.h"

class HapticTester : public Script
{
    DECLARE_SCRIPT(HapticTester)

public:
    explicit HapticTester(GameObject* owner);

    void Start()  override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

public:
    int m_playerIndex = 1;
    int m_hapticDeviceIndex = 0; 

    std::string m_namedEffectId = "Impact";

private:
    uint32_t m_activeHandle = 0;
};