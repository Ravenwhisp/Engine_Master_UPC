#pragma once
#include "Globals.h"
#include "BoundingBox.h"

// :: for global namespace, since DirectX::BoundingBox also exists

bool ::BoundingBox::isPointInsidePlane(const Vector3& point, const Plane& plane) 
{
	return plane.Normal().Dot(point) + plane.D() < 0;
}

bool ::BoundingBox::isFullyOutsideOfPlane(const Plane& plane) 
{
	for (auto& point : m_points) {
		if (isPointInsidePlane(point, plane)) {
			return false;
		}
	}
	return true;
}

bool ::BoundingBox::test(const Frustum& frustum) 
{
	return !isFullyOutsideOfPlane(frustum.m_frontFace)
		&& !isFullyOutsideOfPlane(frustum.m_backFace)
		&& !isFullyOutsideOfPlane(frustum.m_rightFace)
		&& !isFullyOutsideOfPlane(frustum.m_leftFace)
		&& !isFullyOutsideOfPlane(frustum.m_topFace)
		&& !isFullyOutsideOfPlane(frustum.m_bottomFace);
}

void ::BoundingBox::render() 
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

void ::BoundingBox::update(const Matrix& world)
{
	m_points[0] = Vector3::Transform(Vector3(m_min.x, m_min.y, m_min.z), world);
	m_points[1] = Vector3::Transform(Vector3(m_min.x, m_min.y, m_max.z), world);
	m_points[2] = Vector3::Transform(Vector3(m_min.x, m_max.y, m_min.z), world);
	m_points[3] = Vector3::Transform(Vector3(m_min.x, m_max.y, m_max.z), world);
	m_points[4] = Vector3::Transform(Vector3(m_max.x, m_min.y, m_min.z), world);
	m_points[5] = Vector3::Transform(Vector3(m_max.x, m_min.y, m_max.z), world);
	m_points[6] = Vector3::Transform(Vector3(m_max.x, m_max.y, m_min.z), world);
	m_points[7] = Vector3::Transform(Vector3(m_max.x, m_max.y, m_max.z), world);
}