#include "Globals.h"
#include "UIImage.h"
#include <imgui.h>

#include "Application.h"
#include "AssetsModule.h"

UIImage::UIImage(UID id, GameObject* owner): Component(id, ComponentType::UIIMAGE, owner)
{
}

void UIImage::drawUi()
{
    ImGui::Text("UIImage");

    ImGui::Button("Drop Here the Texture");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
        {
            const UID* data = static_cast<const UID*>(payload->Data);
            m_textureAssetId = *data;
            m_texture = nullptr;
            m_textureAsset = static_cast<TextureAsset*>(app->getAssetModule()->requestAsset(*data));
            if (m_textureAsset)
            {
                m_loadRequested = true;
            }
        }
        ImGui::EndDragDropTarget();
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