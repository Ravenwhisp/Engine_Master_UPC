#pragma once
#include "Component.h"
#include "ModuleFont.h"

class UIText : public Component
{
private:
    std::string m_text = "New Label";
    float m_scale = 1.0f;
    DirectX::XMFLOAT4 m_color = { 1,1,1,1 };

    std::string m_font;
    int m_fontId = UNKNOWN_FONT_ID;

    void resolveFontId();

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
};