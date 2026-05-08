#pragma once

#include "Globals.h"

class GameObject;
class MeshRenderer;

struct PickCandidate
{
    GameObject* gameObject = nullptr;
    MeshRenderer* meshRenderer = nullptr;
    float distance = FLT_MAX;
};

namespace ScenePicking
{
    bool intersectMeshRendererAABB(MeshRenderer* meshRenderer, const Ray& worldRay, float& outDistance);
}