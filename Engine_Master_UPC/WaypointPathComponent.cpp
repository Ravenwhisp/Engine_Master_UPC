#include "Globals.h"

#include "WaypointPathComponent.h"
#include "JsonArchive.h"

#include "GameObject.h"
#include "Transform.h"
#include "Application.h"

using namespace DirectX::SimpleMath;

std::unique_ptr<Component> WaypointPathComponent::clone(GameObject* newOwner) const
{
	auto c = std::make_unique<WaypointPathComponent>(m_uuid, newOwner);
	c->setActive(this->isActive());
	c->m_waypoints = m_waypoints;
	return c;
}

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
    JsonArchive archive(ArchiveMode::Output);
    serialize(archive);
    return archive.extractValue(domTree.GetAllocator());
}

bool WaypointPathComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
    JsonArchive archive(ArchiveMode::Input);
    archive.setValue(componentInfo);
    serialize(archive);

    if (componentInfo.HasMember("Waypoints"))
    {
        const auto& arr = componentInfo["Waypoints"];
        for (auto& v : arr.GetArray())
        {
            Vector3 p(v[0].GetFloat(), v[1].GetFloat(), v[2].GetFloat());
            addWaypoint(p);
        }
    }

    return true;
}

void WaypointPathComponent::serialize(IArchive& archive)
{
    if (archive.mode() == ArchiveMode::Output)
    {
        uint64_t uid = m_uuid;
        archive.serialize(uid, "UID");
        uint32_t type = static_cast<uint32_t>(ComponentType::WAYPOINT_PATH);
        archive.serialize(type, "ComponentType");
    }

    bool active = isActive();
    archive.serialize(active, "Active");
    if (archive.mode() == ArchiveMode::Input)
        setActive(active);

    uint32_t count = static_cast<uint32_t>(m_waypoints.size());
    archive.serialize(count, "WaypointCount");
    if (archive.mode() == ArchiveMode::Input)
        m_waypoints.resize(count);

    for (auto& wp : m_waypoints)
    {
        archive.serialize(wp, "Waypoint");
    }
}