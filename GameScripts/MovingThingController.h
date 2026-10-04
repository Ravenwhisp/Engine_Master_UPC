#pragma once

#include "ScriptAPI.h"

class MovingThingController : public Script
{
    DECLARE_SCRIPT(MovingThingController)

public:
    explicit MovingThingController(GameObject* owner);

    FieldList getExposedFields() const override;

    void Start() override;
    void Update() override;

	void drawGizmo() override;
	void setIsOnIdle(bool isOnIdle) { m_isOnIdle = isOnIdle; }

	void setFleeDirection(const Vector2& direction) { m_fleeDirection = direction; }
	Vector2 getFleeDirection() const { return m_fleeDirection; }

	float getDetectionRadius() const { return m_detectionRadius; }
	float getFleeSpeed() const { return m_fleeSpeed; }

	const AssetId& getDisappearParticlesId() const { return m_disappearParticlesPrefab.m_id; }

	Transform* getLyrielTransform() const { return m_lyrielTransform.getReferencedComponent(); }
	Transform* getDeathTransform() const { return m_deathTransform.getReferencedComponent(); }

private:

	float m_detectionRadius = 5.0f;
	float m_fleeSpeed = 3.0f;

	PrefabRef m_disappearParticlesPrefab;

	ComponentRef<Transform> m_lyrielTransform;
	ComponentRef<Transform> m_deathTransform;

	Vector2 m_fleeDirection = Vector2::Zero;

	bool m_isOnIdle = true;
};