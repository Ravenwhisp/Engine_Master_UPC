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
	void setFollowTargets();
	Vector3 lerpVector(const Vector3& start, const Vector3& end, float alpha) const;

	UID m_firstTargetUid = 0;
	UID m_secondTargetUid = 0;
	Transform* m_firstTargetTransform = nullptr;
	Transform* m_secondTargetTransform = nullptr;

	Vector3 m_transformOffset = Vector3::Zero;
	Vector3 m_rotationOffset = Vector3::Zero;

	float m_followSharpness = 12.0f;
};