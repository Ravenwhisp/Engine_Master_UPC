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
    strcpy_s(buf, m_path.c_str());

    if (ImGui::InputText("Path", buf, IM_ARRAYSIZE(buf)))
    {
        m_path = buf;
        m_dirty = true;
    }

    ImGui::Text("Loaded: %s", (m_texture != nullptr) ? "YES" : "NO");
}