#include "Globals.h"
#include "EnemyRenderBufferComponent.h"

#include "JsonArchive.h"

EnemyRenderBufferComponent::EnemyRenderBufferComponent(UID id, GameObject* owner) : Component(id, ComponentType::ENEMY_RENDER_BUFFER, owner)
{
}

std::unique_ptr<Component> EnemyRenderBufferComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<EnemyRenderBufferComponent> newComponent = std::make_unique<EnemyRenderBufferComponent>(m_uuid, newOwner);

	newComponent->m_damageHighlight = m_damageHighlight;
	newComponent->m_damageHighlightData = m_damageHighlightData;

	newComponent->m_dissolveEffect = m_dissolveEffect;
	newComponent->m_dissolveEffectData = m_dissolveEffectData;

	return newComponent;
}

void EnemyRenderBufferComponent::drawUi()
{
	if (ImGui::CollapsingHeader("Damage Highlight Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Highlight Value", &m_damageHighlight, 0.1f, 0.0f, 1.0f);

		float centerColor[3] = { m_damageHighlightData.centerColor.x, m_damageHighlightData.centerColor.y, m_damageHighlightData.centerColor.z };
		if (ImGui::ColorEdit3("Highlight Center Color", centerColor))
		{
			m_damageHighlightData.centerColor = Vector3(centerColor[0], centerColor[1], centerColor[2]);
		}

		float rimColor[3] = { m_damageHighlightData.rimColor.x, m_damageHighlightData.rimColor.y, m_damageHighlightData.rimColor.z };
		if (ImGui::ColorEdit3("Highlight Rim Color", rimColor))
		{
			m_damageHighlightData.rimColor = Vector3(rimColor[0], rimColor[1], rimColor[2]);
		}

		ImGui::DragFloat("Highlight Rim Intensity", &m_damageHighlightData.rimIntensity, 0.1f, 0.1f, 25.0f);
	}
	
	ImGui::Spacing();

	if (ImGui::CollapsingHeader("Dissolve Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Dissolve Value", &m_dissolveEffect, 0.1f, 0.0f, 1.0f);

		float rimColor[3] = { m_dissolveEffectData.rimColor.x, m_dissolveEffectData.rimColor.y, m_dissolveEffectData.rimColor.z };
		if (ImGui::ColorEdit3("Dissolve Rim Color", rimColor))
		{
			m_dissolveEffectData.rimColor = Vector3(rimColor[0], rimColor[1], rimColor[2]);
		}

		ImGui::DragFloat("Dissolve Rim Intensity", &m_dissolveEffectData.rimIntensity, 0.1f, 0.1f, 25.0f);
	}
}

void EnemyRenderBufferComponent::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.serialize(m_damageHighlight, "DamageHighlight");
	archive.serialize(m_damageHighlightData.centerColor, "DamageHighlightCenterColor");
	archive.serialize(m_damageHighlightData.rimColor, "DamageHighlightRimColor");
	archive.serialize(m_damageHighlightData.rimIntensity, "DamageHighlightRimIntensity");

	archive.serialize(m_dissolveEffect, "DissolveEffect");
	archive.serialize(m_dissolveEffectData.rimColor, "DissolveEffectRimColor");
	archive.serialize(m_dissolveEffectData.rimIntensity, "DissolveEffectRimIntensity");
}