#include "Globals.h"
#include "ScenePicking.h"

#include "MeshRenderer.h"
#include "BoundingBox.h"

//Need the conersion because our BB is determined by min, max and 8 points and DirectX one is determined by center and extents, which is what Ray needs.
DirectX::BoundingBox toDirectXBoundingBox(const Engine::BoundingBox& box)
{
    const Vector3* points = box.getPoints();

    Vector3 min = points[0];
    Vector3 max = points[0];

    for (int i = 1; i < 8; ++i)
    {
        min.x = std::min(min.x, points[i].x);
        min.y = std::min(min.y, points[i].y);
        min.z = std::min(min.z, points[i].z);

        max.x = std::max(max.x, points[i].x);
        max.y = std::max(max.y, points[i].y);
        max.z = std::max(max.z, points[i].z);
    }

    const Vector3 center = (min + max) * 0.5f;
    const Vector3 extents = (max - min) * 0.5f;

    return DirectX::BoundingBox(center, extents);
}


namespace ScenePicking
{
    bool intersectMeshRendererAABB(MeshRenderer* meshRenderer, const Ray& worldRay, float& outDistance)
    {
        if (!meshRenderer || !meshRenderer->isActive() || !meshRenderer->hasMesh())
        {
            return false;
        }

        DirectX::BoundingBox boundingBox = toDirectXBoundingBox(meshRenderer->getBoundingBox());

        float distance = 0.0f;

        if (!worldRay.Intersects(boundingBox, distance))
        {
            return false;
        }

        outDistance = distance;
        return true;
    }
}