#include "Globals.h"
#include "NavModifierVolumeComponent.h"
#include "JsonArchive.h"
#include "GameObject.h"
#include "Transform.h"

NavModifierVolumeComponent::NavModifierVolumeComponent(UID id, GameObject* owner)
	: Component(id, ComponentType::NAVMODIFIER_VOLUME, owner)
{
}

std::unique_ptr<Component> NavModifierVolumeComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<NavModifierVolumeComponent> newComponent = std::make_unique<NavModifierVolumeComponent>(m_uuid, newOwner);

	newComponent->m_halfExtents = m_halfExtents;
	newComponent->m_areaType = m_areaType;
	newComponent->m_enabled = m_enabled;
	newComponent->m_priority = m_priority;

	return newComponent;
}

void NavModifierVolumeComponent::drawUi()
{
	ImGui::SeparatorText("NavModifier Volume");

	ImGui::Checkbox("Affects Navigation", &m_enabled);

	ImGui::DragFloat3("Half Extents", &m_halfExtents.x, 0.1f, 0.01f);

	const char* areaTypes[] = { "Default", "Spectral", "Blocked" };
	int currentArea = static_cast<int>(m_areaType);
	if (ImGui::Combo("Area Type", &currentArea, areaTypes, IM_ARRAYSIZE(areaTypes)))
	{
		m_areaType = static_cast<NavAreaType>(currentArea);
	}

	ImGui::DragInt("Priority", &m_priority, 1.0f, 0, 100);
}

void NavModifierVolumeComponent::onTransformChange()
{
	// useful for dynamic navmesh if we do it
}

rapidjson::Value NavModifierVolumeComponent::getJSON(rapidjson::Document& domTree)
{
    JsonArchive archive(ArchiveMode::Output);
    serialize(archive);
    return archive.extractValue(domTree.GetAllocator());
}

bool NavModifierVolumeComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
    JsonArchive archive(ArchiveMode::Input);
    archive.setValue(componentInfo);
    serialize(archive);

    return true;
}

void NavModifierVolumeComponent::serialize(IArchive& archive)
{
	if (archive.mode() == ArchiveMode::Output)
	{
		uint64_t uid = m_uuid;
		archive.serialize(uid, "UID");
		uint32_t type = static_cast<uint32_t>(ComponentType::NAVMODIFIER_VOLUME);
		archive.serialize(type, "ComponentType");
	}

	bool active = isActive();
	archive.serialize(active, "Active");
	if (archive.mode() == ArchiveMode::Input)
		setActive(active);

	archive.beginObject("HalfExtents");
	archive.serialize(m_halfExtents.x, "x");
	archive.serialize(m_halfExtents.y, "y");
	archive.serialize(m_halfExtents.z, "z");
	archive.endObject();

	uint32_t areaType = static_cast<uint32_t>(m_areaType);
	archive.serialize(areaType, "AreaType");
	if (archive.mode() == ArchiveMode::Input)
		m_areaType = static_cast<NavAreaType>(areaType);

	archive.serialize(m_enabled, "Enabled");

	uint32_t priority = static_cast<uint32_t>(m_priority);
	archive.serialize(priority, "Priority");
	if (archive.mode() == ArchiveMode::Input)
		m_priority = static_cast<int>(priority);
}

void NavModifierVolumeComponent::debugDraw()
{
	if (!m_enabled)
		return;

	Transform* transform = getOwner()->GetTransform();

	if (!transform)
		return;

	Vector3 position = transform->getPosition();

	Vector3 min = position - m_halfExtents;
	Vector3 max = position + m_halfExtents;

	if (m_areaType == NavAreaType::Default)
		dd::aabb(&min.x, &max.x, dd::colors::Green);

	if (m_areaType == NavAreaType::Spectral)
		dd::aabb(&min.x, &max.x, dd::colors::Blue);

	if (m_areaType == NavAreaType::Blocked)
		dd::aabb(&min.x, &max.x, dd::colors::Red);
}