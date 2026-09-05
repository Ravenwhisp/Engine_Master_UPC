#pragma once

#include "ScriptAPI.h"

class AelorinDetectionAggro : public Script
{
	DECLARE_SCRIPT(AelorinDetectionAggro)

public:
	explicit AelorinDetectionAggro(GameObject* owner);

	void Start() override;
	void Update() override;
	void drawGizmo() override;

	FieldList getExposedFields() const override;

public:
	float m_detectionRadius = 20.0f;
	bool m_debugEnabled = true;
	ComponentRef<Transform> m_lyrielTransform;
	ComponentRef<Transform> m_deathTransform;

public:
	Transform* getLyrielTransform() const;
	Transform* getDeathTransform() const;

	Vector3 getLyrielPosition() const;
	Vector3 getDeathPosition() const;

	float getDistanceToLyriel() const;
	float getDistanceToDeath() const;

	bool isLyrielInDetectionRange() const;
	bool isDeathInDetectionRange() const;

	bool startEncounter() const;

private:
	Transform* m_lyrielCachedTransform = nullptr;
	Transform* m_deathCachedTransform = nullptr;
	// add boss controller to check phases

private:
	void findPlayerTransforms();
	Transform* getOwnerTransform() const;
	Vector3 getOwnerPosition() const;
};