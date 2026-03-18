#pragma once

#include "BoundingBox.h"

namespace Engine
{
    struct Frustum
    {
        static constexpr int NUM_PLANES = 6;

        Plane m_frontFace;
        Plane m_backFace;
        Plane m_topFace;
        Plane m_bottomFace;
        Plane m_leftFace;
        Plane m_rightFace;

        Vector3 m_points[8]{};

        void render(const Matrix& world) const;

        void calculateFrustumVerticesFromFrustum(const Matrix& world, float horizontalFov, float nearPlane, float farPlane, float aspectRatio, Vector3 verts[8]);

        Plane* getPlanes() { return &m_frontFace; }
        const Plane* getPlanes() const { return &m_frontFace; }
    };
}