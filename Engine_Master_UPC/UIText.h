#pragma once
#include "Component.h"

class UIText : public Component
{
public:
    UIText(UID id, GameObject* owner);

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    const std::string& getText() const { return m_text; }
    void setText(const std::string& text) { m_text = text; m_wideTextDirty = true; }
    const std::wstring& getWideText() {
        if (m_wideTextDirty) {
            m_cachedWideText = stringToWString(m_text);
            m_wideTextDirty = false;
        }
        return m_cachedWideText;
    }

    float getFontScale() const { return m_scale; }
    void setFontScale(float scale) { m_scale = scale; }

    const DirectX::XMFLOAT4& getColor() const { return m_color; }
    void setColor(const DirectX::XMFLOAT4& c) { m_color = c; }

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
    std::string m_text = "New Label";
    std::wstring m_cachedWideText;
    bool m_wideTextDirty = true;
    float m_scale = 1.0f;
    DirectX::XMFLOAT4 m_color = { 1,1,1,1 };

    static std::wstring stringToWString(const std::string& string);
};
