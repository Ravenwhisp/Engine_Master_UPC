#include "Globals.h"
#include "DissolveComponent.h"

#include "JsonArchive.h"

#include "Application.h"
#include "ModuleAssets.h"
#include "ModuleResources.h"

DissolveComponent::DissolveComponent(UID id, GameObject* owner) : Component(id, ComponentType::DISSOLVE, owner)
{
}

std::unique_ptr<Component> DissolveComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<DissolveComponent> newComponent = std::make_unique<DissolveComponent>(m_uuid, newOwner);

	newComponent->m_dissolveData = m_dissolveData;
	
	newComponent->m_textureAssetId = m_textureAssetId;
	newComponent->m_texture = m_texture;
	newComponent->m_textureAsset = m_textureAsset;
	newComponent->m_loadRequested = m_loadRequested;

	return newComponent;
}

void DissolveComponent::drawUi()
{
	if (ImGui::CollapsingHeader("Dissolve Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Dissolve Amount", &m_dissolveData.dissolveAmount, 0.1f, 0.0f, 1.0f);

		float dissolveColor[3] = { m_dissolveData.dissolveColor.x, m_dissolveData.dissolveColor.y, m_dissolveData.dissolveColor.z };
		if (ImGui::ColorEdit3("Dissolve Color", dissolveColor))
		{
			m_dissolveData.dissolveColor = Vector3(dissolveColor[0], dissolveColor[1], dissolveColor[2]);
		}

		ImGui::DragFloat("Dissolve Thikness", &m_dissolveData.dissolveThickness, 0.1f, 0.1f, 5.0f);

		ImGui::Button("Drop Here the Noise Texture");

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
			{
				UID* data = static_cast<UID*>(payload->Data);
				LoadTexture(data);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();
		ImGui::Text("Loaded: %s", (m_texture != nullptr) ? "YES" : "NO");
	}
}

void DissolveComponent::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.serialize(m_dissolveData.dissolveAmount, "DissolveAmount");
	archive.serialize(m_dissolveData.dissolveColor, "DissolveColor");
	archive.serialize(m_dissolveData.dissolveThickness, "DissolveTickness");

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
}

bool DissolveComponent::consumeLoadRequest()
{
	const bool was = m_loadRequested;
	m_loadRequested = false;
	return was;
}

void DissolveComponent::setTextureAssetId(const AssetReference& assetId)
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

void DissolveComponent::LoadTexture(UID* data)
{
	AssetReference* ref = app->getModuleAssets()->findReference(*data);
	m_textureAssetId = *ref;
	m_textureAsset = app->getModuleAssets()->load<TextureAsset>(*ref);
	if (m_textureAsset)
	{
		m_loadRequested = true;
	}

	if (!m_textureAsset || !m_textureAssetId.isValid())
	{
		m_texture = nullptr;
	}
	else
	{
		auto texture = app->getModuleResources()->createTexture(*m_textureAsset, true);
		if (texture)
		{
			m_texture = texture;
		}
		else
		{
			m_texture = nullptr;
		}
	}
}
