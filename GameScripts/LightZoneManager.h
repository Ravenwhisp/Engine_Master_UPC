#pragma once
#include "ScriptAPI.h"

class LightZoneManager : public Script
{
    DECLARE_SCRIPT(LightZoneManager)

public:
    explicit LightZoneManager(GameObject* owner);

    void Start() override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;

    void onBothPlayersCrossed();

    const char* interiorLightName = "InteriorDirectionalLight";
    const char* exteriorLightName = "ExteriorDirectionalLight";

private:
    GameObject* m_interiorLight = nullptr;
    GameObject* m_exteriorLight = nullptr;

    bool m_isExterior = false;

    void applyLightState();
};

