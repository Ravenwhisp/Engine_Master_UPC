#pragma once
#include "Globals.h"
#include "BoundingVolume.h"
#include "Frustum.h"

class Engine::BoundingBox : public BoundingVolume
{
public:
	BoundingBox() {}
	BoundingBox(const Vector3& min, const Vector3& max) : m_min(min), m_max(max) {}
	BoundingBox(const Vector3& min, const Vector3& max, Vector3 points[8]);

	bool isPointInsidePlane(const Vector3& point, const Plane& plane) const;

	bool isFullyOutsideOfPlane(const Plane& plane) const;

	bool test(const Engine::Frustum& frustum) const;

	const Vector3* getPoints() const { return m_points; }
	void setPoint(const int index, const Vector3& point) { m_points[index] = point; }

	void setMin(const Vector3& min) { m_min = min; }
	void setMax(const Vector3& max) { m_max = max; }

	const Vector3& getMin() const { return m_min; }
	const Vector3& getMax() const { return m_max; }

	const Vector3& getMinInWorldSpace() const { return m_points[0]; }
	const Vector3& getMaxInWorldSpace() const { return m_points[7]; }

	void update(const Matrix& world);
	void render() override;

protected:
	// Do NOT update m_min and m_max ever. They conserve the original data of the bounding box
	// (this is tied to the model, whose vertices never change either, they just get transformed).
	// If you need them in world space, use getMinInWorldSpace() and getMaxInWorldSpace()
	Vector3 m_min;
	Vector3 m_max;

	// Cache the points in world space for performance. Only updates when the Transform changes
	Vector3 m_points[8];
};