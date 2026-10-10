#include "Globals.h"
#include "NavModifierVolumeComponent.h"
#include "JsonArchive.h"
#include "GameObject.h"
#include "Transform.h"

NavModifierVolumeComponent::NavModifierVolumeComponent(UID id, GameObject* owner)
	: Component(id, ComponentType::NAV_MODIFIER_VOLUME, owner)
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

	const char* areaTypes[] = { "Default", "Spectral", "Blocked", "DashGap"};
	int currentArea = static_cast<int>(m_areaType);
	if (ImGui::Combo("Area Type", &currentArea, areaTypes, IM_ARRAYSIZE(areaTypes)))
	{
		m_areaType = static_cast<NavAreaType>(currentArea);
	}

	ImGui::DragInt("Priority", &m_priority, 1.0f, 0, 100);
}



void NavModifierVolumeComponent::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.beginObject("HalfExtents");
	archive.serialize(m_halfExtents.x, "x");
	archive.serialize(m_halfExtents.y, "y");
	archive.serialize(m_halfExtents.z, "z");
	archive.endObject();

	archive.serializeStringEnum(m_areaType, "AreaType", NavAreaTypeToString, StringToNavAreaType);

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

	const Matrix worldMatrix = transform->getGlobalMatrix();
	const Vector3& h = m_halfExtents;

	// Box corners in local space
	const Vector3 localCorners[8] =
	{
		{ -h.x, -h.y, -h.z },
		{  h.x, -h.y, -h.z },
		{  h.x, -h.y,  h.z },
		{ -h.x, -h.y,  h.z },

		{ -h.x,  h.y, -h.z },
		{  h.x,  h.y, -h.z },
		{  h.x,  h.y,  h.z },
		{ -h.x,  h.y,  h.z }
	};

	// Transform corners into world space
	Vector3 worldCorners[8];

	for (int i = 0; i < 8; ++i)
	{
		worldCorners[i] = Vector3::Transform(localCorners[i], worldMatrix);
	}

	const float* color = dd::colors::Green;

	switch (m_areaType)
	{
	case NavAreaType::Spectral:
		color = dd::colors::Blue;
		break;

	case NavAreaType::Blocked:
		color = dd::colors::Red;
		break;

	case NavAreaType::DashGap:
		color = dd::colors::Yellow;
		break;

	default:
		break;
	}

	// 12 edges of the box
	const int edges[12][2] =
	{
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	for (const auto& edge : edges)
	{
		dd::line(
			ddConvert(worldCorners[edge[0]]),
			ddConvert(worldCorners[edge[1]]),
			color
		);
	}
	//Vector3 position = transform->getGlobalMatrix().Translation();

	//Vector3 min = position - m_halfExtents;
	//Vector3 max = position + m_halfExtents;

	//if (m_areaType == NavAreaType::Default)
	//	dd::aabb(&min.x, &max.x, dd::colors::Green);

	//if (m_areaType == NavAreaType::Spectral)
	//	dd::aabb(&min.x, &max.x, dd::colors::Blue);

	//if (m_areaType == NavAreaType::Blocked)
	//	dd::aabb(&min.x, &max.x, dd::colors::Red);

	//if(m_areaType == NavAreaType::DashGap)
	//	dd::aabb(&min.x, &max.x, dd::colors::Yellow);
}