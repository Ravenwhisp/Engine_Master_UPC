#pragma once
#include "Component.h"
#include "UID.h"

class GameObject;
class Transform;

class CameraFollow final : public Component {
public:
	CameraFollow(UID id, GameObject* gameObject);

	void update() override;
	void drawUi() override;

private:
	void setFollowTarget();
	Vector3 lerpVector(const Vector3& start, const Vector3& end, float alpha) const;

	UID m_targetUid = 0;
	Transform* m_targetTransform = nullptr;

	Vector3 m_transformOffset = Vector3::Zero;
	Vector3 m_rotationOffset = Vector3::Zero;

	float m_followSharpness = 12.0f;
};