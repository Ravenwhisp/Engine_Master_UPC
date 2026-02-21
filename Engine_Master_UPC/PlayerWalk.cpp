#include "Globals.h"
#include "PlayerWalk.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "TimeModule.h"
#include "InputModule.h"

static const float PI = 3.1415926535897931f;

PlayerWalk::PlayerWalk(UID id, GameObject* gameobject) :
	Component(id, ComponentType::PLAYER_WALK, gameobject) 
{
}

float PlayerWalk::getDeltaSecondsFromTimer() const
{
	return app->getTimeModule()->deltaTime(); 
}

void PlayerWalk::update() {
	Transform* transform = m_owner->GetTransform();

	InputModule* inputModule = app->getInputModule();

	if (!inputModule->isLeftMouseDown()) {
		return;
	}

	Vector3 direction = readMoveDirection(inputModule);


	if (direction == Vector3::Zero)
		return;

	direction.Normalize();

	const float dt = getDeltaSecondsFromTimer();
	bool shiftHeld = checkShiftHeld(inputModule);
	applyFacingFromDirection(transform, direction, dt);
	applyTranslation(transform, direction, dt, shiftHeld);
}

Vector3 PlayerWalk::readMoveDirection(InputModule* inputModule) const
{
	Vector3 direction(0, 0, 0);

	if (inputModule->isKeyDown(m_keyUp)) {
		direction.z -= 1.0f;
	}
	if (inputModule->isKeyDown(m_keyDown)) {
		direction.z += 1.0f;
	}
	if (inputModule->isKeyDown(m_keyLeft)) {
		direction.x -= 1.0f;
	}
	if (inputModule->isKeyDown(m_keyRight)) {
		direction.x += 1.0f;
	}

	return direction;
}

void PlayerWalk::applyFacingFromDirection(Transform* transform, const Vector3& direction, float dt)
{
	const float yawRad = std::atan2(-direction.x, -direction.z);
	const float targetYawDeg = yawRad * (180.0f / PI) + 90.0f;

	if (!m_yawInitialized)
	{
		m_currentYawDeg = transform->getEulerDegrees().y; 
		m_yawInitialized = true;
	}

	const float maxStep = m_turnSpeedDegPerSec * dt;
	m_currentYawDeg = moveTowardsAngleDegrees(m_currentYawDeg, targetYawDeg, maxStep);

	transform->setRotationEuler(Vector3(0.0f, m_currentYawDeg, 0.0f));
}

void PlayerWalk::applyTranslation(Transform* transform, const Vector3& direction, float dt, bool shiftHeld) const
{
	float speed = m_moveSpeed;

	if (shiftHeld) {
		speed *=  m_shiftMultiplier;
	}

	float step = speed * dt;

	Vector3 pos = transform->getPosition();
	pos += direction * step;

	transform->setPosition(pos);
}

bool PlayerWalk::checkShiftHeld(InputModule* inputModule) const{
	const bool shiftDown = inputModule->isKeyDown(Keyboard::Keys::LeftShift) || inputModule->isKeyDown(Keyboard::Keys::RightShift);
	
	return shiftDown;
}

float PlayerWalk::wrapAngleDegrees(float angle)
{
	while (angle > 180.0f) 
	{
		angle -= 360.0f;
	}
	while (angle < -180.0f) 
	{
		angle += 360.0f;
	}
	return angle;
}

float PlayerWalk::moveTowardsAngleDegrees(float currentYawAngle, float targetYawAngle, float maxDelta)
{
	float delta = wrapAngleDegrees(targetYawAngle - currentYawAngle);

	if (delta > maxDelta) 
	{
		delta = maxDelta;
	}

	if (delta < -maxDelta) 
	{
		delta = -maxDelta;
	}

	return currentYawAngle + delta;
}

void PlayerWalk::drawUi() {
	ImGui::Text("Hold Left Mouse Button + WASD to move arround");
	ImGui::Text("Press shift to go faster");

	ImGui::Separator();

	ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.05f, 0.0f, 50.0f);
	ImGui::DragFloat("Shift Multiplier", &m_shiftMultiplier, 0.05f, 1.0f, 10.0f);
}

rapidjson::Value PlayerWalk::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value componentInfo(rapidjson::kObjectType);

	componentInfo.AddMember("UID", m_uuid, domTree.GetAllocator());
	componentInfo.AddMember("ComponentType", ComponentType::PLAYER_WALK, domTree.GetAllocator());

	componentInfo.AddMember("MoveSpeed", m_moveSpeed, domTree.GetAllocator());
	componentInfo.AddMember("ShiftMultiplier", m_shiftMultiplier, domTree.GetAllocator());

	return componentInfo;
}

