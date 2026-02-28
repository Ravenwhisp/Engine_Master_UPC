#include "Globals.h"
#include "UIImage.h"
#include <imgui.h>

UIImage::UIImage(UID id, GameObject* owner)
    : Component(id, ComponentType::UIIMAGE, owner)
{
}

void UIImage::drawUi()
{
    ImGui::Text("UIImage");

    char buf[260];
    buf[0] = '\0';
    if (!m_path.empty())
        strcpy_s(buf, m_path.c_str());

    if (ImGui::InputText("Path", buf, IM_ARRAYSIZE(buf)))
    {
        m_path = buf;
    }

    if (ImGui::Button("Load Texture"))
    {
        m_loadRequested = true;
    }

    ImGui::SameLine();
    ImGui::Text("Loaded: %s", (m_texture != nullptr) ? "YES" : "NO");
}


bool UIImage::consumeLoadRequest()
{
    const bool was = m_loadRequested;
    m_loadRequested = false;
    return was;
}