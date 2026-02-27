#include "Globals.h"
#include "Frustum.h"

void Engine::Frustum::render(const Matrix& world)
{
	float color[3] = { 0.3f, 0.3f, 0.3f };

	dd::line(&m_points[0].x, &m_points[1].x, color);
	dd::line(&m_points[1].x, &m_points[2].x, color);
	dd::line(&m_points[2].x, &m_points[3].x, color);
	dd::line(&m_points[3].x, &m_points[0].x, color);
	dd::line(&m_points[4].x, &m_points[5].x, color);
	dd::line(&m_points[5].x, &m_points[6].x, color);
	dd::line(&m_points[6].x, &m_points[7].x, color);
	dd::line(&m_points[7].x, &m_points[4].x, color);
	dd::line(&m_points[0].x, &m_points[4].x, color);
	dd::line(&m_points[1].x, &m_points[5].x, color);
	dd::line(&m_points[2].x, &m_points[6].x, color);
	dd::line(&m_points[3].x, &m_points[7].x, color);

	// I leave this commented for debugging purposes, I don't want to re-think this
	// if I need to debug the frustum later on
	/*float leftFaceColor[3] = { 0.0f, 0.0f, 0.0f };
	Vector3 leftFaceCenter = ((m_points[0] + m_points[3]) / 2 + (m_points[4] + m_points[7]) / 2) / 2;
	Vector3 leftFaceNormal = m_leftFace.Normal();
	dd::plane(&leftFaceCenter.x, &leftFaceNormal.x, leftFaceColor, leftFaceColor, 0.5f, 1.0f);

	float rightFaceColor[3] = { 0.0f, 0.0f, 1.0f };
	Vector3 rightFaceCenter = ((m_points[1] + m_points[2]) / 2 + (m_points[5] + m_points[6]) / 2) / 2;
	Vector3 rightFaceNormal = m_rightFace.Normal();
	dd::plane(&rightFaceCenter.x, &rightFaceNormal.x, rightFaceColor, rightFaceColor, 0.5f, 1.0f);

	float topFaceColor[3] = { 0.0f, 1.0f, 0.0f};
	Vector3 topFaceCenter = ((m_points[0] + m_points[1]) / 2 + (m_points[4] + m_points[5]) / 2) / 2;
	Vector3 topFaceNormal = m_topFace.Normal();
	dd::plane(&topFaceCenter.x, &topFaceNormal.x, topFaceColor, topFaceColor, 0.5f, 1.0f);

	float bottomFaceColor[3] = { 0.0f, 1.0f, 1.0f };
	Vector3 bottomFaceCenter = ((m_points[3] + m_points[2]) / 2 + (m_points[7] + m_points[6]) / 2) / 2;
	Vector3 bottomFaceNormal = m_bottomFace.Normal();
	dd::plane(&bottomFaceCenter.x, &bottomFaceNormal.x, bottomFaceColor, bottomFaceColor, 0.5f, 1.0f);

	float frontFaceColor[3] = { 1.0f, 0.0f, 0.0f };
	Vector3 frontFaceCenter = ((m_points[0] + m_points[1]) / 2 + (m_points[3] + m_points[2]) / 2) / 2;
	Vector3 frontFaceNormal = m_frontFace.Normal();
	dd::plane(&frontFaceCenter.x, &frontFaceNormal.x, frontFaceColor, frontFaceColor, 0.5f, 1.0f);

	float backFaceColor[3] = { 1.0f, 0.0f, 1.0f };
	Vector3 backFaceCenter = ((m_points[4] + m_points[7]) / 2 + (m_points[5] + m_points[6]) / 2) / 2;
	Vector3 backFaceNormal = m_backFace.Normal();
	dd::plane(&backFaceCenter.x, &backFaceNormal.x, backFaceColor, backFaceColor, 0.5f, 1.0f);*/
}

void Engine::Frustum::calculateFrustumVerticesFromFrustum(const Matrix& world, const float horizontalFov, const float nearPlane, const float farPlane, const float aspectRatio, Vector3 verts[8])
{
	float halfHFovRad = horizontalFov * 0.5f * IM_PI / 180.0f;
	float halfVFovRad = atan(tan(halfHFovRad) / aspectRatio);

	float nh = nearPlane * tan(halfVFovRad);
	float nw = nearPlane * tan(halfHFovRad);
	float fh = farPlane * tan(halfVFovRad);
	float fw = farPlane * tan(halfHFovRad);

	// Corners in camera space
	Vector3 points[8] = {
		{-nw,  nh, nearPlane}, // top left
		{ nw,  nh, nearPlane}, // top right
		{ nw, -nh, nearPlane}, // bottom right
		{-nw, -nh, nearPlane}, // bottom left
		{-fw,  fh, farPlane}, // top left
		{ fw,  fh, farPlane}, // top right
		{ fw, -fh, farPlane}, // bottom right
		{-fw, -fh, farPlane} // bottom left
	};

	// Transform to world space
	for (auto& v : points) v = Vector3::Transform(v, world);

	for (unsigned int i = 0; i < 8; i++) 
	{
		verts[i] = points[i];
	}
}