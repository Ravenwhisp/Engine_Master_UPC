#pragma once

#include "Globals.h"

class MeshRenderer;

namespace ScenePicking
{
    bool intersectMeshRendererAABB(MeshRenderer* meshRenderer, const Ray& worldRay, float& outDistance);
}