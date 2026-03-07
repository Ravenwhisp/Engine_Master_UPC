#include "Globals.h"

#include "NavigationAgentComponent.h"
#include "WaypointPathComponent.h"
#include "Application.h"
#include "NavigationModule.h"
#include "GameObject.h"
#include "Transform.h"
#include "TimeModule.h"

using namespace DirectX::SimpleMath;

bool NavigationAgentComponent::init()
{
	m_running = m_autoStart;
	return true;
}

std::unique_ptr<Component> NavigationAgentComponent::clone(GameObject* newOwner) const
{
	return std::unique_ptr<Component>();
}

void NavigationAgentComponent::setTarget(const Vector3& target)
{
	auto* navigationModule = app->getNavigationModule();
	if (!navigationModule || !navigationModule->hasNavMesh())
		return;

	if (!navigationModule->getNavQuery())
	{
		DEBUG_ERROR("NavQuery is NULL\n");
		return;
	}

	Vector3 start = m_owner->GetTransform()->getGlobalMatrix().Translation();

	std::vector<Vector3> newPath;

	if (!navigationModule->findStraightPath(start, target, newPath, { 5, 5, 5 }))
		return;

	m_path = newPath;
	m_currentIndex = 1;
	m_hasPath = true;
}

void NavigationAgentComponent::update()
{

	if (!m_running)
		return;

	auto* waypointPath = m_owner->GetComponentAs<WaypointPathComponent>(ComponentType::WAYPOINT_PATH);

	if (m_running && waypointPath)
	{
		const auto& points = waypointPath->getWaypoints();

		if (!points.empty())
		{
			Vector3 pos = m_owner->GetTransform()->getGlobalMatrix().Translation();
			Vector3 target = points[m_currentWaypoint];

			if (!m_hasPath)
			{
				setTarget(target);
			}

			float dist = Vector3::Distance(pos, target);

			if (dist < m_waypointReachDistance)
			{
				m_currentWaypoint++;

				if (m_currentWaypoint >= points.size())
					m_currentWaypoint = 0;

				setTarget(points[m_currentWaypoint]);
			}
		}
	}

	if (!m_hasPath || m_currentIndex >= m_path.size())
		return;

	auto* transform = m_owner->GetTransform();

	Vector3 current = transform->getGlobalMatrix().Translation();
	Vector3 target = m_path[m_currentIndex];
	Vector3 direction = target - current;

	float distance = direction.Length();

	if (distance < 0.1f)
	{
		m_currentIndex++;

		if (m_currentIndex >= m_path.size())
			m_hasPath = false;

		return;
	}

	direction.Normalize();

	float dt = app->getTimeModule()->deltaTime();
	current += direction * m_speed * dt;

	// rotate towards the target

	if (m_faceMovement)
	{
		if (direction.LengthSquared() > 0.0001f)
		{
			Matrix world = transform->getGlobalMatrix();

			Vector3 scale;
			Quaternion rotation;
			Vector3 position;

			world.Decompose(scale, rotation, position);

			Vector3 forward = Vector3::Transform(Vector3::UnitX, rotation);
			forward.y = 0.0f;

			if (forward.LengthSquared() < 0.0001f)
				forward = Vector3(0, 0, 1);

			forward.Normalize();

			Vector3 desiredDir = direction;
			desiredDir.y = 0.0f;
			desiredDir.Normalize();

			float dot = forward.Dot(desiredDir);

			if (dot > 1.f) dot = 1.f;
			if (dot < -1.f) dot = -1.f;

			float angle = std::acos(dot);

			if (angle > 0.001f)
			{
				Vector3 cross = forward.Cross(desiredDir);

				float sign = (cross.y > 0) ? 1.f : -1.f;

				float maxStep = m_turnSpeed * dt;
				float step = (angle > maxStep) ? maxStep : angle;

				Quaternion deltaRot =
					Quaternion::CreateFromAxisAngle(Vector3::Up, step * sign);

				rotation = deltaRot * rotation;
			}

			Matrix newWorld =
				Matrix::CreateScale(scale) *
				Matrix::CreateFromQuaternion(rotation) *
				Matrix::CreateTranslation(position);

			transform->setFromGlobalMatrix(newWorld);
		}
	}

	// end of rotation logic

	Matrix world = transform->getGlobalMatrix();
	world.Translation(current);
	transform->setFromGlobalMatrix(world);

}

void NavigationAgentComponent::drawUi()
{
	ImGui::SeparatorText("Navigation Agent");

	ImGui::Checkbox("Draw Path", &m_drawPath);
	
	ImGui::DragFloat("Speed", &m_speed, 0.1f, 0.0f, 20.0f);
	
	ImGui::DragFloat("Turn Speed", &m_turnSpeed, 0.1f, 0.0f, 20.0f);

	ImGui::Checkbox("Face Movement", &m_faceMovement);

	ImGui::Checkbox("Auto Start", &m_autoStart);

	if (ImGui::Button(m_running ? "Stop" : "Start"))
	{
		m_running = !m_running;

		if (!m_running)
		{
			reset();
		}
	}
}

void NavigationAgentComponent::drawDebugPath()
{
	if (!m_drawPath)
		return;

	if (m_path.size() < 2)
		return;

	for (size_t i = 0; i + 1 < m_path.size(); ++i)
	{
		dd::line(ddConvert(m_path[i]), ddConvert(m_path[i + 1]), dd::colors::Yellow);
	}

	dd::sphere(ddConvert(m_path.back()), dd::colors::Green, 0.12f);
}

void NavigationAgentComponent::reset()
{
	m_hasPath = false;
	m_currentIndex = 0;
	m_currentWaypoint = 0;
	m_path.clear();
}

rapidjson::Value NavigationAgentComponent::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value componentInfo(rapidjson::kObjectType);

	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", unsigned int(ComponentType::NAVIGATION_AGENT), domTree.GetAllocator());
	componentInfo.AddMember("Active", this->isActive(), domTree.GetAllocator());
	componentInfo.AddMember("DrawPath", m_drawPath, domTree.GetAllocator());
	componentInfo.AddMember("AutoStart", m_autoStart, domTree.GetAllocator());

	componentInfo.AddMember("Speed", m_speed, domTree.GetAllocator());
	componentInfo.AddMember("TurnSpeed", m_turnSpeed, domTree.GetAllocator());
	componentInfo.AddMember("FaceMovement", m_faceMovement, domTree.GetAllocator());

	return componentInfo;
}

bool NavigationAgentComponent::deserializeJSON(const rapidjson::Value& componentInfo)
{
	if (componentInfo.HasMember("DrawPath")) 
	{
		m_drawPath = componentInfo["DrawPath"].GetBool();
	}
		
	if (componentInfo.HasMember("AutoStart")) 
	{
		m_autoStart = componentInfo["AutoStart"].GetBool();
	}
		
	m_running = m_autoStart;

	if (componentInfo.HasMember("Speed")) 
	{
		m_speed = componentInfo["Speed"].GetFloat();
	}
		
	if (componentInfo.HasMember("TurnSpeed")) 
	{
		m_turnSpeed = componentInfo["TurnSpeed"].GetFloat();
	}
		
	if (componentInfo.HasMember("FaceMovement")) 
	{
		m_faceMovement = componentInfo["FaceMovement"].GetBool();
	}

		

	return true;
}