#pragma once
#include <cstdint>
class GameObject;
class Scene;

using UID = uint64_t;

namespace HierarchyUtils
{
    GameObject* findByUID(Scene* scene, UID id);

    void reparent(Scene* scene, GameObject* child, GameObject* newParent);

    Scene* resolveTargetScene();
}