#include "Globals.h"
#include "UIImage.h"
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
    rapidjson::Value componentInfo(rapidjson::kObjectType);

    componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
    componentInfo.AddMember("ComponentType", int(ComponentType::UIIMAGE), domTree.GetAllocator());
    componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

    componentInfo.AddMember("TextureAssetId", m_textureAssetId.getJson(domTree.GetAllocator()), domTree.GetAllocator());
    componentInfo.AddMember("FillAmount", m_fillAmount, domTree.GetAllocator());
    componentInfo.AddMember("FillMethod", static_cast<int>(m_fillMethod), domTree.GetAllocator());
    componentInfo.AddMember("FillOrigin", static_cast<int>(m_fillOrigin), domTree.GetAllocator());

    componentInfo.AddMember("SheetColumns", m_sheetColumns, domTree.GetAllocator());
    componentInfo.AddMember("SheetRows", m_sheetRows, domTree.GetAllocator());
    {
        rapidjson::Value offset(rapidjson::kArrayType);
        offset.PushBack(m_sheetOffset.x, domTree.GetAllocator());
        offset.PushBack(m_sheetOffset.y, domTree.GetAllocator());
        componentInfo.AddMember("SheetOffset", offset, domTree.GetAllocator());
    }

    componentInfo.AddMember("StretchDrawMode", static_cast<int>(m_stretchDrawMode), domTree.GetAllocator());

    return componentInfo;
}

bool UIImage::deserializeJSON(const rapidjson::Value& componentInfo)
{
    if (componentInfo.HasMember("TextureAssetId"))
    {
        m_textureAssetId.deserializeJson(componentInfo["TextureAssetId"]);

        m_texture = nullptr;
        m_textureAsset = app->getModuleAssets()->load<TextureAsset>(m_textureAssetId);

        if (m_textureAsset)
        {
            m_loadRequested = true;
        }
    }

    if (componentInfo.HasMember("FillAmount"))
        m_fillAmount = componentInfo["FillAmount"].GetFloat();

    if (componentInfo.HasMember("FillMethod"))
        m_fillMethod = static_cast<FillMethod>(componentInfo["FillMethod"].GetInt());

    if (componentInfo.HasMember("FillOrigin"))
    {
        m_fillOrigin = static_cast<FillOrigin>(componentInfo["FillOrigin"].GetInt());
    }

    if (componentInfo.HasMember("SheetColumns"))
        m_sheetColumns = std::max(1, componentInfo["SheetColumns"].GetInt());
    else
        m_sheetColumns = 1;

    if (componentInfo.HasMember("SheetRows"))
        m_sheetRows = std::max(1, componentInfo["SheetRows"].GetInt());
    else
        m_sheetRows = 1;

    if (componentInfo.HasMember("SheetOffset"))
    {
        m_sheetOffset.x = componentInfo["SheetOffset"][0].GetFloat();
        m_sheetOffset.y = componentInfo["SheetOffset"][1].GetFloat();
    }
    else
    {
        m_sheetOffset = { 0.0f, 0.0f };
    }

    if (componentInfo.HasMember("StretchDrawMode"))
    {
        m_stretchDrawMode = static_cast<StretchDrawMode>(componentInfo["StretchDrawMode"].GetInt());
    }
    else
    {
        m_stretchDrawMode = StretchDrawMode::Stretch;
    }

    return true;
}