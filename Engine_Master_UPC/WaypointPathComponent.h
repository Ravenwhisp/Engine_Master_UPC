#pragma once

#include "Component.h"
#include <vector>
#include <SimpleMath.h>

#include <rapidjson/document.h>

class WaypointPathComponent : public Component
{
public:

	WaypointPathComponent(UID id, GameObject* gameObject) : Component(id, ComponentType::WAYPOINT_PATH, gameObject) {}

	const std::vector<DirectX::SimpleMath::Vector3>& getWaypoints() const { return m_waypoints; }

	void addWaypoint(const DirectX::SimpleMath::Vector3& p);
	void clear();

#pragma region Loop
	bool init() override { return true; }
#pragma endregion

	void drawUi() override;
	void drawWaypoints();

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:

	std::vector<DirectX::SimpleMath::Vector3> m_waypoints;
};