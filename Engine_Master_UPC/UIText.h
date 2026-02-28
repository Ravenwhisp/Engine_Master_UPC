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

    const DirectX::XMFLOAT4& getColor() const { return m_color; }
    void setColor(const DirectX::XMFLOAT4& c) { m_color = c; }

    void drawUi() override;

private:
    std::string m_text = "New Label";
    float m_scale = 1.0f;
    DirectX::XMFLOAT4 m_color = { 1,1,1,1 };
};
