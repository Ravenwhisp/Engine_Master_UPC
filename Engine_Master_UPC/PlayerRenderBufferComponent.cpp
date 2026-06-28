#include "Globals.h"
#include "PlayerRenderBufferComponent.h"

#include "JsonArchive.h"


PlayerRenderBufferComponent::PlayerRenderBufferComponent(UID id, GameObject* owner) : Component(id, ComponentType::PLAYER_RENDER_BUFFER, owner)
{
}

std::unique_ptr<Component> PlayerRenderBufferComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<PlayerRenderBufferComponent> newComponent = std::make_unique<PlayerRenderBufferComponent>(m_uuid, newOwner);

	newComponent->m_damageHighlight = m_damageHighlight;
	newComponent->m_damageHighlightData = m_damageHighlightData;

	return newComponent;
}

void PlayerRenderBufferComponent::drawUi()
{
	if (ImGui::CollapsingHeader("Damage Highlight Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Highlight Value", &m_damageHighlight, 0.1f, 0.0f, 1.0f);
		
		float centerColor[3] = { m_damageHighlightData.centerColor.x, m_damageHighlightData.centerColor.y, m_damageHighlightData.centerColor.z };
		if (ImGui::ColorEdit3("Center Color", centerColor))
		{
			m_damageHighlightData.centerColor = Vector3(centerColor[0], centerColor[1], centerColor[2]);
		}

		float rimColor[3] = { m_damageHighlightData.rimColor.x, m_damageHighlightData.rimColor.y, m_damageHighlightData.rimColor.z };
		if (ImGui::ColorEdit3("Rim Color", rimColor))
		{
			m_damageHighlightData.rimColor = Vector3(rimColor[0], rimColor[1], rimColor[2]);
		}
		
		ImGui::DragFloat("Rim Intensity", &m_damageHighlightData.rimIntensity, 0.1f, 0.1f, 25.0f);
	}
}

void PlayerRenderBufferComponent::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.serialize(m_damageHighlight, "DamageHighlight");
	archive.serialize(m_damageHighlightData.centerColor, "CenterColor");
	archive.serialize(m_damageHighlightData.rimColor, "RimColor");
	archive.serialize(m_damageHighlightData.rimIntensity, "RimIntensity");
}