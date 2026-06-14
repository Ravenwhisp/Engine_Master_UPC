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

void UIImage::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.beginObject("TextureAssetId");
	m_textureAssetId.serialize(archive);
	archive.endObject();

	if (archive.mode() == ArchiveMode::Input)
	{
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

	{
		DirectX::SimpleMath::Vector3 v(m_fillAmount.x, m_fillAmount.y, 0.0f);
		archive.serialize(v, "FillAmount");
		if (archive.mode() == ArchiveMode::Input)
		{
			m_fillAmount.x = v.x;
			m_fillAmount.y = v.y;
		}
	}

	archive.serializeStringEnum(m_fillMethod, "FillMethod", FillMethodToString, StringToFillMethod);
	archive.serialize(m_fillOrigin, "FillOrigin");

	{
		uint32_t columns = static_cast<uint32_t>(m_sheetColumns);
		archive.serialize(columns, "SheetColumns");
		if (archive.mode() == ArchiveMode::Input)
			m_sheetColumns = std::max(1, static_cast<int>(columns));
	}

	{
		uint32_t rows = static_cast<uint32_t>(m_sheetRows);
		archive.serialize(rows, "SheetRows");
		if (archive.mode() == ArchiveMode::Input)
			m_sheetRows = std::max(1, static_cast<int>(rows));
	}

	{
		DirectX::SimpleMath::Vector3 v(m_sheetOffset.x, m_sheetOffset.y, 0.0f);
		archive.serialize(v, "SheetOffset");
		if (archive.mode() == ArchiveMode::Input)
		{
			m_sheetOffset.x = v.x;
			m_sheetOffset.y = v.y;
		}
	}

	archive.serializeStringEnum(m_stretchDrawMode, "StretchDrawMode", StretchDrawModeToString, StringToStretchDrawMode);
}

void UIImage::fixReferences(const SceneReferenceResolver& resolver)
{
}