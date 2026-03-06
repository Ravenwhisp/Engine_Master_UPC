#include "Globals.h"

#include "WaypointPathComponent.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"

using namespace DirectX::SimpleMath;

void WaypointPathComponent::addWaypoint(const Vector3& p)
{
	m_waypoints.push_back(p);
}

void WaypointPathComponent::clear()
{
	m_waypoints.clear();
}

void WaypointPathComponent::drawUi()
{
	ImGui::SeparatorText("Waypoint Path");

	ImGui::Text("Waypoints: %d", (int)m_waypoints.size());

	if (ImGui::Button("Add Waypoint"))
	{
		Vector3 pos = m_owner->GetTransform()->getGlobalMatrix().Translation();
		addWaypoint(pos);
	}

	ImGui::SameLine();

	if (ImGui::Button("Clear"))
	{
		clear();
	}

	for (int i = 0; i < (int)m_waypoints.size(); ++i)
	{
		ImGui::PushID(i);
		ImGui::DragFloat3("Waypoint", &m_waypoints[i].x, 0.1f);

		if (ImGui::Button("Delete"))
		{
			m_waypoints.erase(m_waypoints.begin() + i);
			ImGui::PopID();
			break;
		}

		ImGui::Separator();
		ImGui::PopID();
	}
}

void WaypointPathComponent::drawWaypoints()
{
	if (m_waypoints.empty())
		return;

	for (size_t i = 0; i + 1 < m_waypoints.size(); ++i)
	{
		dd::line(
			ddConvert(m_waypoints[i]),
			ddConvert(m_waypoints[i + 1]),
			dd::colors::Cyan
		);

		dd::sphere(ddConvert(m_waypoints[i]), dd::colors::Yellow, 0.15f);
	}

	if (!m_waypoints.empty())
	{
		dd::sphere(ddConvert(m_waypoints.back()), dd::colors::Yellow, 0.15f);
	}
}

rapidjson::Value WaypointPathComponent::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value componentInfo(rapidjson::kObjectType);

	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", unsigned(ComponentType::WAYPOINT_PATH), domTree.GetAllocator());
	componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());

	rapidjson::Value arr(rapidjson::kArrayType);

	for (auto& p : m_waypoints)
	{
		rapidjson::Value v(rapidjson::kArrayType);

		v.PushBack(p.x, domTree.GetAllocator());
		v.PushBack(p.y, domTree.GetAllocator());
		v.PushBack(p.z, domTree.GetAllocator());

		arr.PushBack(v, domTree.GetAllocator());
	}

	componentInfo.AddMember("Waypoints", arr, domTree.GetAllocator());

	return componentInfo;
}

bool WaypointPathComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
	if (componentInfo.HasMember("Waypoints"))
	{
		const auto& arr = componentInfo["Waypoints"];

		for (auto& v : arr.GetArray())
		{
			Vector3 p(
				v[0].GetFloat(),
				v[1].GetFloat(),
				v[2].GetFloat());

			addWaypoint(p);
		}
	}

	return true;
}