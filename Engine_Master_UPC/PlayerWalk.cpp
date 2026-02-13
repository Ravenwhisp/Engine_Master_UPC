#include "Globals.h"
#include "PlayerWalk.h"
#include "GameObject.h"
#include "Transform.h"
#include "Application.h"
#include "TimeModule.h"
#include "InputModule.h"

static const float PI = 3.1415926535897931f;

PlayerWalk::PlayerWalk(int id, GameObject* gameobject) :
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

	applyFacingFromDirection(transform, direction);

	const float dt = getDeltaSecondsFromTimer();
	applyTranslation(transform, direction, dt);
}

Vector3 PlayerWalk::readMoveDirection(InputModule* inputModule) const
{
	Vector3 direction(0, 0, 0);

	if (inputModule->isKeyDown(Keyboard::Keys::W)) {
		direction.z -= 1.0f;
	}
	if (inputModule->isKeyDown(Keyboard::Keys::S)) {
		direction.z += 1.0f;
	}
	if (inputModule->isKeyDown(Keyboard::Keys::A)) {
		direction.x -= 1.0f;
	}
	if (inputModule->isKeyDown(Keyboard::Keys::D)) {
		direction.x += 1.0f;
	}

	return direction;
}

void PlayerWalk::applyFacingFromDirection(Transform* transform, const Vector3& direction) const
{
	const float yawRad = std::atan2(-direction.x, -direction.z);
	const float yawDeg = yawRad * (180.0f / PI) + 90.0f;
	transform->setRotationEuler(Vector3(0.0f, yawDeg, 0.0f));
}

void PlayerWalk::applyTranslation(Transform* transform, const Vector3& direction, float dt) const
{
	const float step = m_moveSpeed * dt;

	Vector3 pos = *transform->getPosition();
	pos += direction * step;

	transform->setPosition(pos);
}

