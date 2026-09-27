#pragma once
#include "SimpleMath.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <limits>

namespace ShadowCascadeMath
{
    using DirectX::SimpleMath::Matrix;
    using DirectX::SimpleMath::Vector3;
    struct Bounds
    {
        Vector3 min = Vector3(std::numeric_limits<float>::max());
        Vector3 max = Vector3(-std::numeric_limits<float>::max());
        void include(const Vector3& p)
        {
            min.x = std::min(min.x, p.x); min.y = std::min(min.y, p.y); min.z = std::min(min.z, p.z);
            max.x = std::max(max.x, p.x); max.y = std::max(max.y, p.y); max.z = std::max(max.z, p.z);
        }
    };
    struct Fit
    {
        Matrix viewProjection;
        float worldTexel = 0;
        float depthRange = 0;
        std::vector<size_t> casters;
    };
    inline void cameraCorners(const Matrix& inverseView, const Matrix& projection, float nearDepth, float farDepth, Vector3 (&corners)[8])
    {
        size_t i = 0;
        for (float depth : { nearDepth, farDepth })
            for (float y : { -1.0f, 1.0f })
                for (float x : { -1.0f, 1.0f })
                    corners[i++] = Vector3::Transform(Vector3(x * depth / projection._11, y * depth / projection._22, -depth), inverseView);
    }
    // Bounds are in the fixed light orientation. Include upstream casters even outside the camera.
    inline Fit fit(const Vector3 (&corners)[8], const Matrix& lightOrientation, uint32_t resolution,
        float guardTexels, const std::vector<Bounds>& bounds)
    {
        Vector3 center = Vector3::Zero;
        for (const auto& p : corners) center += p;
        center /= 8.0f;
        float radius = 5.0f;
        for (const auto& p : corners) radius = std::max(radius, Vector3::Distance(center, p));
        radius = std::ceil(radius * 16.0f) / 16.0f;
        const float width = 2.0f * radius / std::max(0.5f, 1.0f - 2.0f * guardTexels / float(resolution));
        Fit result;
        result.worldTexel = width / float(resolution);
        Vector3 lightCenter = Vector3::Transform(center, lightOrientation);
        lightCenter.x = std::round(lightCenter.x / result.worldTexel) * result.worldTexel;
        lightCenter.y = std::round(lightCenter.y / result.worldTexel) * result.worldTexel;
        float nearZ = lightCenter.z + radius;
        const float farZ = lightCenter.z - radius;
        for (size_t i = 0; i < bounds.size(); ++i)
        {
            const auto& b = bounds[i];
            if (b.max.x < lightCenter.x - width * 0.5f || b.min.x > lightCenter.x + width * 0.5f ||
                b.max.y < lightCenter.y - width * 0.5f || b.min.y > lightCenter.y + width * 0.5f || b.max.z < farZ)
                continue;
            result.casters.push_back(i);
            nearZ = std::max(nearZ, b.max.z);
        }
        const float depthPadding = std::max(0.1f, result.worldTexel * 2.0f);
        nearZ += depthPadding;
        result.depthRange = nearZ - farZ + depthPadding;
        Matrix view = lightOrientation;
        view._41 = -lightCenter.x; view._42 = -lightCenter.y; view._43 = -nearZ;
        result.viewProjection = view * Matrix::CreateOrthographic(width, width, 0.0f, result.depthRange);
        return result;
    }
}
