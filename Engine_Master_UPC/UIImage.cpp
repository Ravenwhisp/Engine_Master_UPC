#include "Globals.h"
#include "UIImage.h"
#include <imgui.h>

#include "Application.h"
#include "AssetsModule.h"
#include <UIRect.h>

UIImage::UIImage(UID id, GameObject* owner): Component(id, ComponentType::UIIMAGE, owner)
{
}

bool UIImage::containsPoint(const Rect2D& rect, const Vector2& screenPos) const
{
    if (!m_texture) return true;

    const int texW = m_textureAsset->getWidth();
    const int texH = m_textureAsset->getHeight();

    if (texW <= 0 || texH <= 0) return true;

    // Normalised position inside the rect  [0 .. 1]
    const float u = (screenPos.x - rect.x) / rect.w;
    const float v = (screenPos.y - rect.y) / rect.h;

    // Map to texture pixel coords
    const int px = static_cast<int>(u * static_cast<float>(texW));
    const int py = static_cast<int>(v * static_cast<float>(texH));

    return px >= 0 && px < texW
        && py >= 0 && py < texH;
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