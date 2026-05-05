#include "Globals.h"
#include "NavModifierVolumeComponent.h"

NavModifierVolumeComponent::NavModifierVolumeComponent(UID id, GameObject* owner)
	: Component(id, ComponentType::NAVMODIFIERVOLUME, owner)
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

//void drawUi() override;
//void onTransformChange() override {}
//
rapidjson::Value NavModifierVolumeComponent::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value componentInfo(rapidjson::kObjectType);

	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", unsigned int(ComponentType::NAVMODIFIERVOLUME), domTree.GetAllocator());
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
//bool deserializeJSON(const rapidjson::Value& componentInfo) override;
//
//void debugDraw() override;