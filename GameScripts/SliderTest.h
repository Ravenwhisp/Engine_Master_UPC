#pragma once

#include "ScriptAPI.h"
#include "UISlider.h"

class SliderTest : public Script
{
    DECLARE_SCRIPT(SliderTest)

public:
    explicit SliderTest(GameObject* owner);

    void Start() override;
    void Update() override;
    FieldList getExposedFields() const override;

public:
    ComponentRef<UISlider> m_slider;

    float m_fillSpeed = 0.5f;
    float m_stepAmount = 0.1f;
    bool m_autoTest = false;

private:
    float m_currentFill = 1.0f;
    bool m_increasing = false;
};