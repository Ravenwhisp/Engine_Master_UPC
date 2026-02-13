#include "Globals.h"
#include "Frustum.h"

void Frustum::render(const Matrix& world)
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
}

void Frustum::calculateFrustumVerticesFromFrustum(const Matrix& world, const float horizontalFov, const float nearPlane, const float farPlane, const float aspectRatio, Vector3 verts[8]) 
{
	float tanFov = tan(horizontalFov / aspectRatio * (IM_PI / 180.0f) * 0.5f);

	// Near plane
	float nh = nearPlane * tanFov;
	float nw = nh * aspectRatio;

	// Far plane
	float fh = farPlane * tanFov;
	float fw = fh * aspectRatio;

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