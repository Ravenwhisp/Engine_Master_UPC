#pragma once
#include "Component.h"
#include "UID.h"

class GameObject;
class Transform;

class CameraFollowComponent final : public Component
{
public:
	CameraFollowComponent(UID id, GameObject* gameObject);
	
	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	bool init() override;

	void update() override;
	void drawUi() override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentValue) override;
	void fixReferences(const std::unordered_map<UID, Component*>& referenceMap) override;


private:
	Vector3 computeFollowPoint() const;
	float computeTargetExtraHeight(const Vector3& p1, const Vector3& p2) const;
	float smoothExtraHeight(float current, float target, float sharpness, float dt) const;
	Vector3 computeDesiredCameraPosition(const Vector3& followPoint) const;
	Vector3 smoothCameraPosition(const Vector3& current, const Vector3& target, float sharpness, float dt) const;

	Vector3 lerpVector(const Vector3& start, const Vector3& end, float alpha) const;
	float lerpFloat(float start, float end, float alpha) const;

	bool m_firstUpdateAfterResolve = true;

	UID m_firstTargetTransformUid = 0;
	UID m_secondTargetTransformUid = 0;
	Transform* m_firstTargetTransform = nullptr;
	Transform* m_secondTargetTransform = nullptr;

	Vector3 m_transformOffset = Vector3::Zero;
	Vector3 m_rotationOffset = Vector3::Zero;

	float m_zoomStartDistance = 0.0;
	float m_zoomEndDistance = 0.0;
	float m_maxExtraHeight = 0.0;
	float m_currentExtraHeight = 0.0;

	float m_followSharpness = 12.0f;
	float m_zoomSharpness = 8.0f;
};