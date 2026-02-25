#include "Globals.h"
#include "PlayerWalk.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "TimeModule.h"
#include "InputModule.h"
#include "Logger.h"

static const float PI = 3.1415926535897931f;

PlayerWalk::PlayerWalk(UID id, GameObject* gameobject) :
	Component(id, ComponentType::PLAYER_WALK, gameobject) 
{
	Transform* transform = m_owner->GetTransform();
	m_initialRotationOffset = transform->getEulerDegrees();
}

float PlayerWalk::getDeltaSecondsFromTimer() const
{
	return app->getTimeModule()->deltaTime(); 
}

bool PlayerWalk::init() {
	// Capturing the initial rotation should go in the init when we have script system, but at the moment it never gets called so gotta go on the constructor.

	Transform* transform = m_owner->GetTransform();
	m_initialRotationOffset = transform->getEulerDegrees();

	return true;
}

void PlayerWalk::update() {
	Transform* transform = m_owner->GetTransform();

	InputModule* inputModule = app->getInputModule();

	/*if (!inputModule->isKeyDown(Keyboard::Keys::LeftControl)) {
		return;
	}*/

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
	const float targetYawDeg = yawRad * (180.0f / PI);

	if (!m_yawInitialized)
	{
		m_currentYawDeg = 0.0; 
		m_yawInitialized = true;
	}

	const float maxStep = m_turnSpeedDegPerSec * dt;
	m_currentYawDeg = moveTowardsAngleDegrees(m_currentYawDeg, targetYawDeg, maxStep);

	const float finalYaw = wrapAngleDegrees(m_initialRotationOffset.y + m_currentYawDeg);

	transform->setRotationEuler(Vector3(m_initialRotationOffset.x, finalYaw, m_initialRotationOffset.z));
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

	ImGui::SeparatorText("Controls");

	if (drawControlSchemeCombo(m_controlScheme))
	{
		applyControlScheme();
	}

	ImGui::SeparatorText("Tuning");

	ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.05f, 0.0f, 50.0f);
	ImGui::DragFloat("Shift Multiplier", &m_shiftMultiplier, 0.05f, 1.0f, 10.0f);
}

//Controls

// For now let only choose between two modes, but in the end we probably have to let them choose their keys
void PlayerWalk::applyControlScheme()
{
	switch (m_controlScheme)
	{
	case ControlScheme::ARROWS:
		m_keyUp = Keyboard::Keys::Up;
		m_keyLeft = Keyboard::Keys::Left;
		m_keyDown = Keyboard::Keys::Down;
		m_keyRight = Keyboard::Keys::Right;
		break;

	case ControlScheme::WASD:
	default:
		m_keyUp = Keyboard::Keys::W;
		m_keyLeft = Keyboard::Keys::A;
		m_keyDown = Keyboard::Keys::S;
		m_keyRight = Keyboard::Keys::D;
		break;
	}
}

bool PlayerWalk::drawControlSchemeCombo(ControlScheme& scheme)
{
	bool changed = false;

	if (ImGui::BeginCombo("Control Scheme", controlSchemeToString(scheme)))
	{
		for (int i = 0; i < (int)ControlScheme::COUNT; ++i)
		{
			ControlScheme value = (ControlScheme)i;
			bool selected = (value == scheme);

			if (ImGui::Selectable(controlSchemeToString(value), selected))
			{
				scheme = value;
				changed = true;
			}

			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	return changed;
}

const char* PlayerWalk::controlSchemeToString(ControlScheme scheme)
{
	switch (scheme)
	{
	case ControlScheme::WASD:   return "WASD";
	case ControlScheme::ARROWS: return "Arrow Keys";
	default: return "Unknown";
	}
}
