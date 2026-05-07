#include "Globals.h"
#include "NavModifierVolumeComponent.h"
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
	rapidjson::Value componentInfo(rapidjson::kObjectType);

	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", unsigned int(ComponentType::NAVMODIFIER_VOLUME), domTree.GetAllocator());
	componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

	rapidjson::Value halfExtentsObj(rapidjson::kObjectType);
	halfExtentsObj.AddMember("x", m_halfExtents.x, domTree.GetAllocator());
	halfExtentsObj.AddMember("y", m_halfExtents.y, domTree.GetAllocator());
	halfExtentsObj.AddMember("z", m_halfExtents.z, domTree.GetAllocator());

	componentInfo.AddMember("HalfExtents", halfExtentsObj, domTree.GetAllocator());
	componentInfo.AddMember("AreaType", unsigned int(m_areaType), domTree.GetAllocator());
	componentInfo.AddMember("Enabled", m_enabled, domTree.GetAllocator());
	componentInfo.AddMember("Priority", m_priority, domTree.GetAllocator());

	return componentInfo;
}

bool NavModifierVolumeComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
	if (componentInfo.HasMember("HalfExtents") && componentInfo["HalfExtents"].IsObject())
	{
		const auto& halfExtents = componentInfo["HalfExtents"];

		if (halfExtents.HasMember("x"))
		{
			m_halfExtents.x = halfExtents["x"].GetFloat();
		}

		if (halfExtents.HasMember("y"))
		{
			m_halfExtents.y = halfExtents["y"].GetFloat();
		}

		if (halfExtents.HasMember("z"))
		{
			m_halfExtents.z = halfExtents["z"].GetFloat();
		}
	}

	if (componentInfo.HasMember("AreaType"))
	{
		m_areaType = static_cast<NavAreaType>(componentInfo["AreaType"].GetUint());
	}

	if (componentInfo.HasMember("Enabled"))
	{
		m_enabled = componentInfo["Enabled"].GetBool();
	}

	if (componentInfo.HasMember("Priority"))
	{
		m_priority = componentInfo["Priority"].GetInt();
	}

	return true;
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