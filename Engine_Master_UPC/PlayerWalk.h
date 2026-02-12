#pragma once

#include "Component.h"

class InputModule;
class Transform;
class GameObject;

class PlayerWalk final : public Component {
public:
	PlayerWalk(int id, GameObject* gameObject);

	void update() override;

private:
	float m_moveSpeed = 3.5f;

	Vector3 readMoveDirection(InputModule* inputModule) const;
	void applyFacingFromDirection(Transform* transform, const Vector3& direction) const;
	void applyTranslation(Transform* transform, const Vector3& direction, float dt) const;

	float getDeltaSecondsFromTimer() const;
};
