#pragma once
#include "Component.h"

class UIText : public Component
{
public:
    UIText(UID id, GameObject* owner);

    const std::string& getText() const { return m_text; }
    void setText(const std::string& text) { m_text = text; }

    float getFontScale() const { return m_scale; }
    void setFontScale(float scale) { m_scale = scale; }

    void drawUi() override;

private:
    std::string m_text = "New Label";
    float m_scale = 1.0f;
};
