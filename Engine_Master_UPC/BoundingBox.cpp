#include "Globals.h"
#include "BoundingBox.h"

#include <algorithm>

namespace Engine
{
    const float EPSILON = 0.001f;

    BoundingBox::BoundingBox(const Vector3& min, const Vector3& max)
        : m_min(min), m_max(max)
    {
    }

    BoundingBox::BoundingBox(const Vector3& min, const Vector3& max, const Vector3 points[8])
        : m_min(min), m_max(max)
    {
        std::copy(points, points + 8, m_points);
    }

    bool BoundingBox::isPointInsidePlane(const Vector3& point, const Plane& plane) const
    {
        bool testRes = plane.Normal().Dot(point) + plane.D() >= -EPSILON;
        return testRes;
    }

    bool BoundingBox::isFullyOutsideOfPlane(const Plane& plane) const
    {
        for (int i = 0; i < 8; ++i)
        {
            if (isPointInsidePlane(m_points[i], plane))
            {
                return false;
            }
        }
        return true;
    }

    bool BoundingBox::test(const Frustum& frustum) const
    {
        PERF_LOGIC("testBB");
        if (isFullyOutsideOfPlane(frustum.m_frontFace)) return false;
        if (isFullyOutsideOfPlane(frustum.m_backFace)) return false;
        if (isFullyOutsideOfPlane(frustum.m_leftFace)) return false;
        if (isFullyOutsideOfPlane(frustum.m_rightFace)) return false;
        if (isFullyOutsideOfPlane(frustum.m_topFace)) return false;
        if (isFullyOutsideOfPlane(frustum.m_bottomFace)) return false;

        return true;
    }

    void BoundingBox::update(const Matrix& world)
    {
        m_points[0] = Vector3::Transform(Vector3(m_min.x, m_min.y, m_min.z), world);
        m_points[1] = Vector3::Transform(Vector3(m_max.x, m_min.y, m_min.z), world);
        m_points[2] = Vector3::Transform(Vector3(m_max.x, m_max.y, m_min.z), world);
        m_points[3] = Vector3::Transform(Vector3(m_min.x, m_max.y, m_min.z), world);

        m_points[4] = Vector3::Transform(Vector3(m_min.x, m_min.y, m_max.z), world);
        m_points[5] = Vector3::Transform(Vector3(m_max.x, m_min.y, m_max.z), world);
        m_points[6] = Vector3::Transform(Vector3(m_max.x, m_max.y, m_max.z), world);
        m_points[7] = Vector3::Transform(Vector3(m_min.x, m_max.y, m_max.z), world);
    }

    void BoundingBox::render()
    {
        float color[3] = { 0.3f, 0.3f, 0.3f };

        const Vector3* c = getPoints();
        ddVec3 pts[8];
        for (int i = 0; i < 8; ++i)
        {
            pts[i][0] = c[i].x;
            pts[i][1] = c[i].y;
            pts[i][2] = c[i].z;
        }
        dd::box(pts, color, 0, false);
    }

}