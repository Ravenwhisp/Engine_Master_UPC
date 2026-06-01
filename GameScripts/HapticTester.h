#pragma once
#include "ScriptAPI.h"
#include "HapticEffectDefinition.h" 
#include <vector>

class HapticTester : public Script
{
    DECLARE_SCRIPT(HapticTester)

public:
    explicit HapticTester(GameObject* owner);

    void Start()  override;
    void Update() override;
    void registerTestEffects();

    ScriptFieldList getExposedFields() const override;

protected:
    virtual std::vector<HapticEffectDefinition> defineEffects();

public:
    int m_playerIndex = 1;
    int m_hapticDeviceIndex = 0;
    std::string m_namedEffectId = "Impact";

private:
    uint32_t m_activeHandle = 0;
};