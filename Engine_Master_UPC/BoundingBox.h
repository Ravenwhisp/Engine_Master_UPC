#pragma once
#include "BoundingVolume.h"
#include "Frustum.h"

namespace Engine
{
    class BoundingBox : public BoundingVolume
    {
    public:

        BoundingBox() = default;
        BoundingBox(const Vector3& min, const Vector3& max);
        BoundingBox(const Vector3& min, const Vector3& max, const Vector3 points[8]);

        bool test(const Frustum& frustum) const;

        const Vector3* getPoints() const { return m_points; }

        void setMin(const Vector3& min) { m_min = min; }
        void setMax(const Vector3& max) { m_max = max; }

        const Vector3& getMin() const { return m_min; }
        const Vector3& getMax() const { return m_max; }

        const Vector3& getMinInWorldSpace() const { return m_points[0]; }
        const Vector3& getMaxInWorldSpace() const { return m_points[6]; }

        void update(const Matrix& world);
        void render() override;

    private:

        bool isPointInsidePlane(const Vector3& point, const Plane& plane) const;
        bool isFullyOutsideOfPlane(const Plane& plane) const;

        Vector3 m_min;
        Vector3 m_max;
        Vector3 m_points[8];
    };
}