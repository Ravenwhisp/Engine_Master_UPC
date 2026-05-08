#pragma once

#include "Globals.h"

class GameObject;
class MeshRenderer;

struct GameObjectPickHit
{
    GameObject* gameObject = nullptr;
    MeshRenderer* meshRenderer = nullptr;
    Vector3 hitPoint = Vector3::Zero;
    float distance = FLT_MAX;
};

namespace ScenePicking
{
    bool intersectMeshRendererAABB(MeshRenderer* meshRenderer, const Ray& worldRay, float& outDistance);
    bool intersectMeshRendererTriangles(MeshRenderer* meshRenderer, const Ray& worldRay, float& outDistance, Vector3& outHitPoint);
}