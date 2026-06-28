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

	return newComponent;
}

void PlayerRenderBufferComponent::drawUi()
{
	ImGui::DragFloat("Damage highlight", &m_damageHighlight, 0.1f, 0.0f, 1.0f);
}

void PlayerRenderBufferComponent::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.serialize(m_damageHighlight, "DamageHighlight");
}

void PlayerRenderBufferComponent::setDamageHighlight(float value)
{
	m_damageHighlight = value;
}

float PlayerRenderBufferComponent::getDamageHighlight()
{
	return m_damageHighlight;
}
