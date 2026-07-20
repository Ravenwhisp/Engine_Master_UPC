#pragma once

#include "ScriptAPI.h"

class SkeletonAttackConfig;

class SkeletonAttackDebugDraw : public Script
{
	DECLARE_SCRIPT(SkeletonAttackDebugDraw)

public:
	explicit SkeletonAttackDebugDraw(GameObject* owner);

	void Start() override;
	void drawGizmo() override;

	FieldList getExposedFields() const override;

public:
	bool m_debugEnabled = true;

	bool m_drawScimitarStartRange = true;
	bool m_drawDashStopRange = true;
	bool m_drawScimitarAttackArea = true;
	bool m_drawScimitarStunArea = true;

	float m_heightOffset = 0.15f;

private:
	AssetReference<SkeletonAttackConfig> m_attackConfig;

private:
	void drawScimitarAttackCone() const;
	void drawScimitarStunCone() const;
	void drawScimitarCone(float range, float halfAngleDegrees, const Vector3& color) const;

	Vector3 rotateAroundY(const Vector3& vector, float radians) const;
};