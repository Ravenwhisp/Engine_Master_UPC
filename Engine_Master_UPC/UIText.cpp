#include "Globals.h"
#include "UIText.h"
#include <imgui.h>

UIText::UIText(UID id, GameObject* owner)
    : Component(id, ComponentType::UITEXT, owner)
{
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
}