#include "Globals.h"
#include "UIText.h"
#include "JsonArchive.h"
#include <imgui.h>

UIText::UIText(UID id, GameObject* owner)
    : Component(id, ComponentType::UITEXT, owner)
{
}

void UIText::serialize(IArchive& archive)
{
    if (archive.mode() == ArchiveMode::Output)
    {
        uint64_t uid = m_uuid;
        archive.serialize(uid, "UID");
        uint32_t type = static_cast<uint32_t>(ComponentType::UITEXT);
        archive.serialize(type, "ComponentType");
    }

    bool active = isActive();
    archive.serialize(active, "Active");
    if (archive.mode() == ArchiveMode::Input)
        setActive(active);

    archive.serialize(m_text, "Text");
    archive.serialize(m_scale, "Scale");
    archive.serialize(reinterpret_cast<DirectX::SimpleMath::Color&>(m_color), "Color");
}

std::unique_ptr<Component> UIText::clone(GameObject* newOwner) const
{
    std::unique_ptr<UIText> clonedComponent = std::make_unique<UIText>(m_uuid, newOwner);

    clonedComponent->setActive(this->isActive());
    clonedComponent->setText(m_text);
    clonedComponent->setFontScale(m_scale);
    clonedComponent->setColor(m_color);

	return clonedComponent;
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
}

