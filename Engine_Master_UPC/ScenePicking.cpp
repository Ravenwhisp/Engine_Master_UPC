#include "Globals.h"
#include "ScenePicking.h"

#include "MeshRenderer.h"
#include "BoundingBox.h"
#include "BasicMesh.h"
#include "GameObject.h"
#include "Transform.h"
#include "IndexBuffer.h"

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

uint32_t readIndex(const std::vector<uint8_t>& indices, DXGI_FORMAT format, uint32_t indexPosition)
{
    const int indexSize = getSizeByFormat(format);
    const uint8_t* data = indices.data() + indexPosition * indexSize;

    switch (format)
    {
    case DXGI_FORMAT_R8_UINT:
        return *reinterpret_cast<const uint8_t*>(data);

    case DXGI_FORMAT_R16_UINT:
        return *reinterpret_cast<const uint16_t*>(data);

    case DXGI_FORMAT_R32_UINT:
        return *reinterpret_cast<const uint32_t*>(data);

    default:
        return 0;
    }
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

    bool intersectMeshRendererTriangles(MeshRenderer* meshRenderer, const Ray& worldRay, float& outDistance)
    {
        if (!meshRenderer || !meshRenderer->isActive() || !meshRenderer->hasMesh())
        {
            return false;
        }

        GameObject* owner = meshRenderer->getOwner();

        if (!owner || !owner->IsActiveInWindowHierarchy())
        {
            return false;
        }

        std::shared_ptr<BasicMesh>& mesh = meshRenderer->getMesh();

        if (!mesh || !mesh->hasIndexBuffer())
        {
            return false;
        }

        const std::vector<Vector3>& vertices = mesh->getVertexPositions();
        const std::vector<uint8_t>& indices = mesh->getIndices();
        const std::vector<Submesh>& submeshes = mesh->getSubmeshes();

        if (vertices.empty() || indices.empty() || submeshes.empty())
        {
            return false;
        }

        const DXGI_FORMAT indexFormat = mesh->getIndexBuffer()->getIndexFormat();

        const Matrix worldMatrix = owner->GetTransform()->getGlobalMatrix();
        const Matrix inverseWorldMatrix = worldMatrix.Invert();

        //Ray transformation from world space to local space to match mesh vertices - ray space
        Vector3 localRayOrigin = Vector3::Transform(worldRay.position, inverseWorldMatrix);
        Vector3 localRayDirection = Vector3::TransformNormal(worldRay.direction, inverseWorldMatrix);

        if (localRayDirection.LengthSquared() <= 0.000001f)
        {
            return false;
        }

        localRayDirection.Normalize();

        Ray localRay(localRayOrigin, localRayDirection);
        //

        bool hit = false;
        float closestDistance = FLT_MAX;

        for (const Submesh& submesh : submeshes)
        {
            const uint32_t indexEnd = submesh.indexStart + submesh.indexCount;

            for (uint32_t i = submesh.indexStart; i + 2 < indexEnd; i += 3)
            {
                const uint32_t index0 = readIndex(indices, indexFormat, i);
                const uint32_t index1 = readIndex(indices, indexFormat, i + 1);
                const uint32_t index2 = readIndex(indices, indexFormat, i + 2);

                if (index0 >= vertices.size() || index1 >= vertices.size() || index2 >= vertices.size())
                {
                    continue;
                }

                float triangleDistance = 0.0f;

                if (localRay.Intersects(vertices[index0], vertices[index1], vertices[index2], triangleDistance))
                {
                    if (triangleDistance < closestDistance)
                    {
                        closestDistance = triangleDistance;
                        hit = true;
                    }
                }
            }
        }

        if (!hit)
        {
            return false;
        }

        const Vector3 localHitPoint = localRay.position + localRay.direction * closestDistance;
        const Vector3 worldHitPoint = Vector3::Transform(localHitPoint, worldMatrix);

        outDistance = (worldHitPoint - worldRay.position).Length();

        return true;
    }
}