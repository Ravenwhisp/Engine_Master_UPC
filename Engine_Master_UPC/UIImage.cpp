#include "Globals.h"
#include "UIImage.h"
#include "JsonArchive.h"
#include <imgui.h>

#include "Application.h"
#include "ModuleAssets.h"
#include <UIRect.h>
#include <Transform2D.h>
#include <GameObject.h>

UIImage::UIImage(UID id, GameObject* owner): Component(id, ComponentType::UIIMAGE, owner)
{
}

void UIImage::setTextureAssetId(const AssetReference& assetId)
{
    m_textureAssetId = assetId;
    m_texture = nullptr;
    m_textureAsset.reset();
    m_loadRequested = false;

    if (m_textureAssetId.isValid())
    {
        m_textureAsset = app->getModuleAssets()->load<TextureAsset>(m_textureAssetId);
        if (m_textureAsset)
        {
            m_loadRequested = true;
        }
    }
}

std::unique_ptr<Component> UIImage::clone(GameObject* newOwner) const
{
    std::unique_ptr<UIImage> cloned = std::make_unique<UIImage>(m_uuid, newOwner);

    cloned->m_textureAssetId = m_textureAssetId;
    cloned->m_texture = m_texture;
    cloned->m_textureAsset = m_textureAsset;
    cloned->m_loadRequested = m_loadRequested;

    cloned->m_fillAmount = m_fillAmount;
    cloned->m_fillMethod = m_fillMethod;
    cloned->m_fillOrigin = m_fillOrigin;

    cloned->m_sheetColumns = m_sheetColumns;
    cloned->m_sheetRows = m_sheetRows;
    cloned->m_sheetOffset = m_sheetOffset;

    cloned->m_stretchDrawMode = m_stretchDrawMode;
    return cloned;
}

bool UIImage::containsPoint(const Rect2D& rect, const Vector2& screenPos) const
{
    if (!m_texture) return true;

    const int texW = m_textureAsset->getWidth();
    const int texH = m_textureAsset->getHeight();

    if (texW <= 0 || texH <= 0) return true;

    const float u = (screenPos.x - rect.x) / rect.w;
    const float v = (screenPos.y - rect.y) / rect.h;

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
            UID* data = static_cast<UID*>(payload->Data);
            AssetReference* ref = app->getModuleAssets()->findReference(*data);
            m_textureAssetId = *ref;
            m_texture = nullptr;
            m_textureAsset = app->getModuleAssets()->load<TextureAsset>(*ref);
            if (m_textureAsset)
            {
                m_loadRequested = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    ImGui::Text("Loaded: %s", (m_texture != nullptr) ? "YES" : "NO");

    if (m_texture)
    {
        if (ImGui::Button("Set Native Size"))
        {
            if (Transform2D* transform = getOwner()->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D))
            {
                transform->setBaseSize({ static_cast<float>(m_textureAsset->getWidth()), static_cast<float>(m_textureAsset->getHeight()) });
            }
        }
        ImGui::Separator();
        ImGui::TextUnformatted("Stretch");
        {
            const char* modes[] = { "Stretch", "Tile" };
            int mode = static_cast<int>(m_stretchDrawMode);
            if (ImGui::Combo("When Stretched", &mode, modes, IM_ARRAYSIZE(modes)))
            {
                m_stretchDrawMode = static_cast<StretchDrawMode>(mode);
            }
            ImGui::TextDisabled("Used when Transform2D Stretch Mode is not None.");
        }
    }
}

bool UIImage::consumeLoadRequest()
{
    const bool was = m_loadRequested;
    m_loadRequested = false;
    return was;
}

rapidjson::Value UIImage::getJSON(rapidjson::Document& domTree)
{
    JsonArchive archive(ArchiveMode::Output);
    serialize(archive);
    return archive.extractValue(domTree.GetAllocator());
}

bool UIImage::deserializeJSON(const rapidjson::Value& componentInfo)
{
    JsonArchive archive(ArchiveMode::Input);
    archive.setValue(componentInfo);
    serialize(archive);

    return true;
}

void UIImage::serialize(IArchive& archive)
{
    if (archive.mode() == ArchiveMode::Output)
    {
        uint64_t uid = m_uuid;
        archive.serialize(uid, "UID");
        uint32_t type = static_cast<uint32_t>(ComponentType::UIIMAGE);
        archive.serialize(type, "ComponentType");
    }

    bool active = isActive();
    archive.serialize(active, "Active");
    if (archive.mode() == ArchiveMode::Input)
        setActive(active);

    archive.beginObject("TextureAssetId");
    m_textureAssetId.serialize(archive);
    archive.endObject();

    archive.serialize(m_fillAmount, "FillAmount");

    uint32_t fillMethod = static_cast<uint32_t>(m_fillMethod);
    archive.serialize(fillMethod, "FillMethod");
    if (archive.mode() == ArchiveMode::Input)
        m_fillMethod = static_cast<FillMethod>(fillMethod);

    uint32_t fillOrigin = static_cast<uint32_t>(m_fillOrigin);
    archive.serialize(fillOrigin, "FillOrigin");
    if (archive.mode() == ArchiveMode::Input)
        m_fillOrigin = static_cast<FillOrigin>(fillOrigin);

    uint32_t sheetColumns = static_cast<uint32_t>(m_sheetColumns);
    archive.serialize(sheetColumns, "SheetColumns");
    if (archive.mode() == ArchiveMode::Input)
        m_sheetColumns = static_cast<int>(sheetColumns);

    uint32_t sheetRows = static_cast<uint32_t>(m_sheetRows);
    archive.serialize(sheetRows, "SheetRows");
    if (archive.mode() == ArchiveMode::Input)
        m_sheetRows = static_cast<int>(sheetRows);

    DirectX::SimpleMath::Vector3 sheetOffset(m_sheetOffset.x, m_sheetOffset.y, 0.0f);
    archive.serialize(sheetOffset, "SheetOffset");
    if (archive.mode() == ArchiveMode::Input)
    {
        m_sheetOffset.x = sheetOffset.x;
        m_sheetOffset.y = sheetOffset.y;
    }

    uint32_t stretchDrawMode = static_cast<uint32_t>(m_stretchDrawMode);
    archive.serialize(stretchDrawMode, "StretchDrawMode");
    if (archive.mode() == ArchiveMode::Input)
        m_stretchDrawMode = static_cast<StretchDrawMode>(stretchDrawMode);
}