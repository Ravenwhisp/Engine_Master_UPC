#pragma once

#ifdef ENGINE_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

#include "ScriptTypes.h"
#include "ComponentType.h"
#include "SimpleMath.h"

using DirectX::SimpleMath::Vector3;

class GameObject;
class Transform;
class Component;

ENGINE_API void registerScript(const char* scriptName, ScriptCreator creator);

namespace Time
{
	ENGINE_API float getDeltaTime();
}

namespace Input 
{
	ENGINE_API bool isKeyDown(int key);
}

namespace GameObjectAPI 
{
    ENGINE_API Transform* getTransform(GameObject* gameObject);
    ENGINE_API const Transform* getTransform(const GameObject* gameObject);
    ENGINE_API bool isActiveSelf(const GameObject* gameObject);
    ENGINE_API bool isActiveInHierarchy(const GameObject* gameObject);
    ENGINE_API void setActive(GameObject* gameObject, bool active);
    ENGINE_API const char* getName(const GameObject* gameObject);
}

namespace TransformAPI
{
    ENGINE_API Vector3 getPosition(const Transform* transform);
    ENGINE_API void setPosition(Transform* transform, const Vector3& newPosition);

    ENGINE_API Vector3 getEulerDegrees(const Transform* transform);
    ENGINE_API void setRotationEuler(Transform* transform, const Vector3& eulerDegrees);
}

namespace ComponentAPI
{
    ENGINE_API GameObject* getOwner(Component* component);
    ENGINE_API const GameObject* getOwner(const Component* component);
}

namespace Scene
{
    ENGINE_API int countGameObjectsByComponent(ComponentType componentType, bool onlyActive = true);
    ENGINE_API int findGameObjectsByComponent(ComponentType componentType, GameObject** outputList, int maxResults, bool onlyActive = true);

    ENGINE_API GameObject* getDefaultCameraGameObject();
    ENGINE_API void setDefaultCameraByGameObject(GameObject* gameObject);
}