#pragma once
#include "Component.h"
#include "ModuleFont.h"

#include "UICommands.h"

class UIText : public Component
{
private:
    std::string m_text = "New Label";
    float m_scale = 1.0f;
    DirectX::XMFLOAT4 m_color = { 1,1,1,1 };

    std::string m_font;
    int m_fontId = UNKNOWN_FONT_ID;

    void resolveFontId();

private:
    uint32_t m_effectFlags = UITextEffect_None;

    DirectX::XMFLOAT4 m_outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 m_shadowColor = { 0.0f, 0.0f, 0.0f, 0.6f };
    DirectX::XMFLOAT4 m_glowColor = { 1.0f, 1.0f, 1.0f, 0.5f };

    float m_outlineSize = 1.0f;
    float m_shadowOffsetX = 2.0f;
    float m_shadowOffsetY = 2.0f;
    float m_glowSize = 3.0f;
    float m_waveAmplitude = 2.0f;
    float m_waveFrequency = 0.05f;
    float m_waveSpeed = 4.0f;

public:
    UIText(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    const std::string& getText() const { return m_text; }
    void setText(const std::string& text) { m_text = text; }

    float getFontScale() const { return m_scale; }
    void setFontScale(float scale) { m_scale = scale; }

    const DirectX::XMFLOAT4& getColor() const { return m_color; }
    void setColor(const DirectX::XMFLOAT4& c) { m_color = c; }

    int getFontId();

    const std::string& getFont() const { return m_font; }
    void setFont(const std::string& font);

    void drawUi() override;
    void serialize(IArchive& archive) override;

    UITextCommand buildCommand(const Rect2D& rect);
};