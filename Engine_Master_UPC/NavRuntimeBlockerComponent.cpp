#include "Globals.h"
#include "NavRuntimeBlockerComponent.h"

#include "Application.h"
#include "ModuleNavigation.h"
#include "GameObject.h"
#include "Transform.h"

NavRuntimeBlockerComponent::NavRuntimeBlockerComponent(UID id, GameObject* owner)
	: Component(id, ComponentType::NAV_RUNTIME_BLOCKER, owner)
{
}

std::unique_ptr<Component> NavRuntimeBlockerComponent::clone(GameObject* newOwner) const
{
	std::unique_ptr<NavRuntimeBlockerComponent> newComponent = std::make_unique<NavRuntimeBlockerComponent>(m_uuid, newOwner);

	newComponent->m_halfExtents = m_halfExtents;
	newComponent->m_blocked = m_blocked;

	return newComponent;
}

void NavRuntimeBlockerComponent::drawUi()
{
	ImGui::SeparatorText("Nav Runtime Blocker");

	ImGui::DragFloat3("Half Extents", &m_halfExtents.x, 0.1f, 0.01f);

	bool blocked = m_blocked;
	if (ImGui::Checkbox("Blocked", &blocked))
	{
		setBlocked(blocked);
	}
}

void NavRuntimeBlockerComponent::onTransformChange()
{
}

rapidjson::Value NavRuntimeBlockerComponent::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value componentInfo(rapidjson::kObjectType);

	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", unsigned int(ComponentType::NAV_RUNTIME_BLOCKER), domTree.GetAllocator());
	componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

	rapidjson::Value halfExtentsObj(rapidjson::kObjectType);
	halfExtentsObj.AddMember("x", m_halfExtents.x, domTree.GetAllocator());
	halfExtentsObj.AddMember("y", m_halfExtents.y, domTree.GetAllocator());
	halfExtentsObj.AddMember("z", m_halfExtents.z, domTree.GetAllocator());

	componentInfo.AddMember("HalfExtents", halfExtentsObj, domTree.GetAllocator());
	componentInfo.AddMember("Blocked", m_blocked, domTree.GetAllocator());

	return componentInfo;
}

bool NavRuntimeBlockerComponent::deserializeJSON(const rapidjson::Value& componentInfo)
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

	if (componentInfo.HasMember("Blocked"))
	{
		m_blocked = componentInfo["Blocked"].GetBool();
	}

	return true;
}

void NavRuntimeBlockerComponent::debugDraw()
{
	Transform* transform = getOwner()->GetTransform();
	if (!transform)
	{
		return;
	}

	const Vector3 center = transform->getGlobalMatrix().Translation();

	const float* color = m_blocked ? dd::colors::Red : dd::colors::Green;

	dd::aabb(ddConvert(center - m_halfExtents), ddConvert(center + m_halfExtents), color);
}

void NavRuntimeBlockerComponent::setBlocked(bool blocked)
{
	m_blocked = blocked;
}