#pragma once
#include "Globals.h"
#include "BoundingBox.h"

// :: for global namespace, since DirectX::BoundingBox also exists

Engine::BoundingBox::BoundingBox(const Vector3& min, const Vector3& max, Vector3 points[8]) : m_min(min), m_max(max)
{
	for (int i = 0; i < 8; i++)
	{
		m_points[i] = points[i];
	}
}

bool Engine::BoundingBox::isPointInsidePlane(const Vector3& point, const Plane& plane) const
{
	return plane.Normal().Dot(point) + plane.D() < 0;
}

bool Engine::BoundingBox::isFullyOutsideOfPlane(const Plane& plane) const
{
	for (int i = 0; i < 8; i++)
	{
		if (isPointInsidePlane(m_points[i], plane))
		{
			return false;
		}
	}
	return true;
}

bool Engine::BoundingBox::test(const Engine::Frustum& frustum) const
{
	if (isFullyOutsideOfPlane(frustum.m_frontFace)) return false;
	if (isFullyOutsideOfPlane(frustum.m_backFace)) return false;
	if (isFullyOutsideOfPlane(frustum.m_leftFace)) return false;
	if (isFullyOutsideOfPlane(frustum.m_rightFace)) return false;
	if (isFullyOutsideOfPlane(frustum.m_topFace)) return false;
	if (isFullyOutsideOfPlane(frustum.m_bottomFace)) return false;

	return true;
}

void Engine::BoundingBox::render()
{
	float color[3] = { 0.3f, 0.3f, 0.3f };
	dd::line(&m_points[0].x, &m_points[1].x, color);
	dd::line(&m_points[2].x, &m_points[3].x, color);
	dd::line(&m_points[0].x, &m_points[2].x, color);
	dd::line(&m_points[1].x, &m_points[3].x, color);
	dd::line(&m_points[4].x, &m_points[5].x, color);
	dd::line(&m_points[6].x, &m_points[7].x, color);
	dd::line(&m_points[4].x, &m_points[6].x, color);
	dd::line(&m_points[7].x, &m_points[5].x, color);
	dd::line(&m_points[0].x, &m_points[4].x, color);
	dd::line(&m_points[2].x, &m_points[6].x, color);
	dd::line(&m_points[1].x, &m_points[5].x, color);
	dd::line(&m_points[3].x, &m_points[7].x, color);
}

void Engine::BoundingBox::update(const Matrix& world)
{
	m_points[0] = Vector3::Transform(Vector3(m_min.x, m_min.y, m_min.z), world);
	m_points[1] = Vector3::Transform(Vector3(m_max.x, m_min.y, m_min.z), world);
	m_points[2] = Vector3::Transform(Vector3(m_min.x, m_max.y, m_min.z), world);
	m_points[3] = Vector3::Transform(Vector3(m_max.x, m_max.y, m_min.z), world);
	m_points[4] = Vector3::Transform(Vector3(m_min.x, m_min.y, m_max.z), world);
	m_points[5] = Vector3::Transform(Vector3(m_max.x, m_min.y, m_max.z), world);
	m_points[6] = Vector3::Transform(Vector3(m_min.x, m_max.y, m_max.z), world);
	m_points[7] = Vector3::Transform(Vector3(m_max.x, m_max.y, m_max.z), world);
}