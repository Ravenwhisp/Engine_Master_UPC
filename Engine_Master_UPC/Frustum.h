#pragma once
#include "Globals.h"

struct Frustum {
	Plane m_frontFace;
	Plane m_backFace;
	Plane m_topFace;
	Plane m_bottomFace;
	Plane m_leftFace;
	Plane m_rightFace;

	Vector3 m_points[8] = {};

	void render(const Matrix& world);
	void calculateFrustumVerticesFromFrustum(const Matrix& world, const float horizontalFov, const float nearPlane, const float farPlane, const float aspectRatio, Vector3 verts[8]);
};