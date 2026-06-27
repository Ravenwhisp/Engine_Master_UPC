#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"

class HUDFader : public Script
{
    DECLARE_SCRIPT(HUDFader)

public:
    explicit HUDFader(GameObject* owner);

    void Start() override;
    void Update() override;

    void fadeTo(float targetAlpha, float duration);

    void setAlpha(float alpha);
    float getAlpha() const { return m_currentAlpha; }

private:
    void findFadeTargets();
    void applyAlpha(float alpha);

private:
    std::vector<Transform2D*> m_fadeTargets;

    bool m_isFading = false;

    float m_currentAlpha = 1.0f;
    float m_startAlpha = 1.0f;
    float m_targetAlpha = 1.0f;

    float m_timer = 0.0f;
    float m_duration = 0.0f;
};