#include "Globals.h"
#include "SpectralComponent.h"

#include "JsonArchive.h"

SpectralComponent::SpectralComponent(UID id, GameObject* owner) : Component(id, ComponentType::SPECTRAL, owner)
{
}

std::unique_ptr<Component> SpectralComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<SpectralComponent> newComponent = std::make_unique<SpectralComponent>(m_uuid, newOwner);

	newComponent->m_spectralData = m_spectralData;

	return newComponent;
}

void SpectralComponent::drawUi()
{
	if (ImGui::CollapsingHeader("Spectral Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		float spectralColor[3] = { m_spectralData.spectralColor.x, m_spectralData.spectralColor.y, m_spectralData.spectralColor.z };
		if (ImGui::ColorEdit3("Spectral Color", spectralColor))
		{
			m_spectralData.spectralColor = Vector3(spectralColor[0], spectralColor[1], spectralColor[2]);
		}
	}
}

void SpectralComponent::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.serialize(m_spectralData.spectralColor, "SpectralColor");
}
