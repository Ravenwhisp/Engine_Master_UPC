#pragma once
#include "Globals.h"
#include "BoundingVolume.h"

class BoundingBox : public BoundingVolume
{
public:
	bool isPointInsidePlane(const Vector3& point, const Plane& plane);

	bool isFullyOutsideOfPlane(const Plane& plane);

	bool test(const Frustum& frustum);

	const Vector3* getPoints() const { return m_points; }
	void setPoint(int index, Vector3 point) { m_points[index] = point; }

	void setMin(Vector3 min) { m_min = min; }
	void setMax(Vector3 max) { m_max = max; }

	void update(const Matrix& world);
	void render() override;

protected:
	Vector3 m_min;
	Vector3 m_max;
	Vector3 m_points[8];
};