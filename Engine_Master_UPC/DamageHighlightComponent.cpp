#include "Globals.h"
#include "DamageHighlightComponent.h"

#include "JsonArchive.h"

DamageHighlightComponent::DamageHighlightComponent(UID id, GameObject* owner) : Component(id, ComponentType::DAMAGE_HIGHLIGHT_EFFECT, owner)
{
}

std::unique_ptr<Component> DamageHighlightComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<DamageHighlightComponent> newComponent = std::make_unique<DamageHighlightComponent>(m_uuid, newOwner);

	newComponent->m_damageHighlightData = m_damageHighlightData;

	return newComponent;
}

void DamageHighlightComponent::drawUi()
{
	if (ImGui::CollapsingHeader("Damage Highlight Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Highlight Value", &m_damageHighlightData.damageHighlight, 0.1f, 0.0f, 1.0f);

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

void DamageHighlightComponent::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.serialize(m_damageHighlightData.damageHighlight, "DamageHighlight");
	archive.serialize(m_damageHighlightData.centerColor, "CenterColor");
	archive.serialize(m_damageHighlightData.rimColor, "RimColor");
	archive.serialize(m_damageHighlightData.rimIntensity, "RimIntensity");
}
