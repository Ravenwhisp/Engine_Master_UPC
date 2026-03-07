#pragma once

#include "Component.h"
#include <vector>
#include <SimpleMath.h>

#include <rapidjson/document.h>

class NavigationAgentComponent : public Component
{
public:
	NavigationAgentComponent(UID id, GameObject* gameObject) : Component(id, ComponentType::NAVIGATION_AGENT, gameObject) {}

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void setTarget(const DirectX::SimpleMath::Vector3& target);

	bool init() override;
	void update() override;

	void drawUi() override;
	void drawDebugPath();

	void reset();

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
	std::vector<DirectX::SimpleMath::Vector3> m_path;
	size_t m_currentIndex = 0;

	bool m_hasPath = false;
	bool m_drawPath = true;

	bool m_autoStart = false;
	bool m_running = false;

	float m_speed = 3.0f;
	float m_turnSpeed = 8.0f;
	bool m_faceMovement = true;

	size_t m_currentWaypoint = 0;
	float m_waypointReachDistance = 0.3f;

public:
	const std::vector<Vector3>& getPath() const { return m_path; }
};