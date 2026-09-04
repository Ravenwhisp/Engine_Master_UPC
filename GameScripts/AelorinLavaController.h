#pragma once

#include "ScriptAPI.h"

class AelorinLavaController : public Script
{
	DECLARE_SCRIPT(AelorinLavaController)

public: 
	explicit AelorinLavaController(GameObject* owner);

	FieldList getExposedFields() const override;

	void Update() override;

	void StartLavaRise(float height, float duration);
	void StartLavaFall(float height, float duration);

	ComponentRef<Transform> m_LavaTransform;

private:
	float smoothStep(float t) const;

	Transform* m_lavaTransform = nullptr;

	float m_startHeight = 0.0f;
	float m_targetHeight = 0.0f;
	float m_elapsedTime = 0.0f;
	float m_duration = 1.0f;
	bool m_animating = false;
};

