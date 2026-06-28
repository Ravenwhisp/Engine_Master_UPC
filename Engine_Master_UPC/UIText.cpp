#include "Globals.h"
#include "UIText.h"
#include "JsonArchive.h"
#include <imgui.h>

#include "Application.h"
#include "ModuleFont.h"


UIText::UIText(UID id, GameObject* owner)
    : Component(id, ComponentType::UITEXT, owner)
{
    m_fontId = -1;
}

void UIText::serialize(IArchive& archive)
{
    Component::serialize(archive);
    archive.serialize(m_text, "Text");
    archive.serialize(m_scale, "Scale");
    archive.serialize(reinterpret_cast<DirectX::SimpleMath::Color&>(m_color), "Color");

    archive.serialize(m_font, "Font");

    m_fontId = UNKNOWN_FONT_ID;
}

std::unique_ptr<Component> UIText::clone(GameObject* newOwner) const
{
    std::unique_ptr<UIText> clonedComponent = std::make_unique<UIText>(m_uuid, newOwner);

    clonedComponent->setActive(this->isActive());
    clonedComponent->setText(m_text);
    clonedComponent->setFontScale(m_scale);
    clonedComponent->setColor(m_color);

    clonedComponent->setFont(m_font);

	return clonedComponent;
}

int UIText::getFontId()
{
    resolveFontId();
    return m_fontId;
}

void UIText::resolveFontId()
{
    if (m_fontId != UNKNOWN_FONT_ID)
        return;

    if (m_font.empty())
    {
        m_fontId = INVALID_FONT_ID;
        return;
    }

    m_fontId = app->getModuleFont()->findFontId(m_font);
}

void UIText::setFont(const std::string& font)
{
    m_font = font;
    m_fontId = UNKNOWN_FONT_ID;
}

void UIText::drawUi()
{
    ImGui::Text("UIText");

    char buf[512];
    std::memset(buf, 0, sizeof(buf));

    if (!m_text.empty())
    {
        std::strncpy(buf, m_text.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
    }

    if (ImGui::InputText("Text", buf, IM_ARRAYSIZE(buf)))
    {
        m_text = buf;
    }

    ImGui::DragFloat("Scale", &m_scale, 0.01f, 0.1f, 5.0f);

    float color[4] = { m_color.x, m_color.y, m_color.z, m_color.w };
    if (ImGui::ColorEdit4("Color", color))
    {
        m_color = { color[0], color[1], color[2], color[3] };
    }

    ModuleFont* fontModule = app->getModuleFont();
    const auto& fontNames = fontModule->getFontNames();

    resolveFontId();

    std::string preview;

    if (m_fontId >= 0)
    {
        preview = m_font;
    }
    else if (m_fontId == INVALID_FONT_ID && !m_font.empty())
    {
        preview = "Unknown: " + m_font;
    }
    else
    {
        preview = "<None>";
    }

    if (ImGui::BeginCombo("Font", preview.c_str()))
    {
        for (const auto& fontName : fontNames)
        {
            const bool selected = (m_font == fontName);

            if (ImGui::Selectable(fontName.c_str(), selected))
            {
                m_font = fontName;
                m_fontId = fontModule->findFontId(m_font);
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
}

