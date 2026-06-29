#include "Globals.h"
#include "UIText.h"
#include "JsonArchive.h"
#include <imgui.h>
#include <cfloat>

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

    archive.serialize(m_effectFlags, "EffectFlags");

    archive.serialize(reinterpret_cast<DirectX::SimpleMath::Color&>(m_outlineColor), "OutlineColor");
    archive.serialize(reinterpret_cast<DirectX::SimpleMath::Color&>(m_shadowColor), "ShadowColor");
    archive.serialize(reinterpret_cast<DirectX::SimpleMath::Color&>(m_glowColor), "GlowColor");

    archive.serialize(m_outlineSize, "OutlineSize");
    archive.serialize(m_shadowOffsetX, "ShadowOffsetX");
    archive.serialize(m_shadowOffsetY, "ShadowOffsetY");
    archive.serialize(m_glowSize, "GlowSize");
    archive.serialize(m_waveAmplitude, "WaveAmplitude");
    archive.serialize(m_waveFrequency, "WaveFrequency");
    archive.serialize(m_waveSpeed, "WaveSpeed");

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

    clonedComponent->m_effectFlags = m_effectFlags;
    clonedComponent->m_outlineColor = m_outlineColor;
    clonedComponent->m_shadowColor = m_shadowColor;
    clonedComponent->m_glowColor = m_glowColor;

    clonedComponent->m_outlineSize = m_outlineSize;
    clonedComponent->m_shadowOffsetX = m_shadowOffsetX;
    clonedComponent->m_shadowOffsetY = m_shadowOffsetY;
    clonedComponent->m_glowSize = m_glowSize;
    clonedComponent->m_waveAmplitude = m_waveAmplitude;
    clonedComponent->m_waveFrequency = m_waveFrequency;
    clonedComponent->m_waveSpeed = m_waveSpeed;

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

    char buf[2048];
    std::memset(buf, 0, sizeof(buf));

    if (!m_text.empty())
    {
        std::strncpy(buf, m_text.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
    }

    if (ImGui::InputTextMultiline("Text", buf, IM_ARRAYSIZE(buf), ImVec2(-FLT_MIN, 90.0f)))
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

    ImGui::SeparatorText("Effects");

    bool outline = (m_effectFlags & UITextEffect_Outline) != 0;
    bool shadow = (m_effectFlags & UITextEffect_Shadow) != 0;
    bool glow = (m_effectFlags & UITextEffect_Glow) != 0;
    bool wave = (m_effectFlags & UITextEffect_Wave) != 0;

    if (ImGui::Checkbox("Outline", &outline))
    {
        if (outline) m_effectFlags |= UITextEffect_Outline;
        else m_effectFlags &= ~UITextEffect_Outline;
    }

    if (outline)
    {
        float outlineColor[4] = { m_outlineColor.x, m_outlineColor.y, m_outlineColor.z, m_outlineColor.w };
        if (ImGui::ColorEdit4("Outline Color", outlineColor))
            m_outlineColor = { outlineColor[0], outlineColor[1], outlineColor[2], outlineColor[3] };

        ImGui::DragFloat("Outline Size", &m_outlineSize, 0.05f, 0.0f, 10.0f);
    }

    if (ImGui::Checkbox("Shadow", &shadow))
    {
        if (shadow) m_effectFlags |= UITextEffect_Shadow;
        else m_effectFlags &= ~UITextEffect_Shadow;
    }

    if (shadow)
    {
        float shadowColor[4] = { m_shadowColor.x, m_shadowColor.y, m_shadowColor.z, m_shadowColor.w };
        if (ImGui::ColorEdit4("Shadow Color", shadowColor))
            m_shadowColor = { shadowColor[0], shadowColor[1], shadowColor[2], shadowColor[3] };

        ImGui::DragFloat("Shadow Offset X", &m_shadowOffsetX, 0.1f, -50.0f, 50.0f);
        ImGui::DragFloat("Shadow Offset Y", &m_shadowOffsetY, 0.1f, -50.0f, 50.0f);
    }

    if (ImGui::Checkbox("Glow", &glow))
    {
        if (glow) m_effectFlags |= UITextEffect_Glow;
        else m_effectFlags &= ~UITextEffect_Glow;
    }

    if (glow)
    {
        float glowColor[4] = { m_glowColor.x, m_glowColor.y, m_glowColor.z, m_glowColor.w };
        if (ImGui::ColorEdit4("Glow Color", glowColor))
            m_glowColor = { glowColor[0], glowColor[1], glowColor[2], glowColor[3] };

        ImGui::DragFloat("Glow Size", &m_glowSize, 0.1f, 0.0f, 20.0f);
    }

    if (ImGui::Checkbox("Wave", &wave))
    {
        if (wave) m_effectFlags |= UITextEffect_Wave;
        else m_effectFlags &= ~UITextEffect_Wave;
    }

    if (wave)
    {
        ImGui::DragFloat("Wave Amplitude", &m_waveAmplitude, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Wave Frequency", &m_waveFrequency, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Wave Speed", &m_waveSpeed, 0.1f, 0.0f, 20.0f);
    }
}

UITextCommand UIText::buildCommand(const Rect2D& rect)
{
    UITextCommand command;

    command.text = std::wstring(m_text.begin(), m_text.end());
    command.x = rect.x;
    command.y = rect.y;
    command.color = m_color;
    command.scale = m_scale;
    command.fontId = getFontId();

    command.effectFlags = m_effectFlags;

    command.outlineColor = m_outlineColor;
    command.shadowColor = m_shadowColor;
    command.glowColor = m_glowColor;

    command.outlineSize = m_outlineSize;
    command.shadowOffsetX = m_shadowOffsetX;
    command.shadowOffsetY = m_shadowOffsetY;
    command.glowSize = m_glowSize;
    command.waveAmplitude = m_waveAmplitude;
    command.waveFrequency = m_waveFrequency;
    command.waveSpeed = m_waveSpeed;

    return command;
}
