#pragma once

#ifdef ENGINE_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

#include "ScriptCreator.h"
#include "ComponentType.h"
#include "Tag.h"
#include "SimpleMath.h"
#include "KeyCode.h"

using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector2;

class GameObject;
class Transform;
class Component;

ENGINE_API void registerScript(const char* scriptName, ScriptCreator creator);

namespace GameObjectAPI 
{
    ENGINE_API Transform* getTransform(GameObject* gameObject);
    ENGINE_API const Transform* getTransform(const GameObject* gameObject);

    ENGINE_API bool isActiveSelf(const GameObject* gameObject);
    ENGINE_API bool isActiveInHierarchy(const GameObject* gameObject);
    ENGINE_API void setActive(GameObject* gameObject, bool active);

    ENGINE_API const char* getName(const GameObject* gameObject);

    ENGINE_API Tag getTag(const GameObject* gameObject);
    ENGINE_API void setTag(GameObject* gameObject, Tag tag);
}

namespace TransformAPI
{
    ENGINE_API Vector3 getPosition(const Transform* transform);
    ENGINE_API void setPosition(Transform* transform, const Vector3& newPosition);

    ENGINE_API Vector3 getScale(const Transform* transform);
    ENGINE_API void setScale(Transform* transform, const Vector3& newScale);

    ENGINE_API Vector3 getEulerDegrees(const Transform* transform);
    ENGINE_API void setRotationEuler(Transform* transform, const Vector3& eulerDegrees);

    ENGINE_API Vector3 getForward(const Transform* transform);
    ENGINE_API Vector3 getRight(const Transform* transform);
    ENGINE_API Vector3 getUp(const Transform* transform);
    ENGINE_API void translate(Transform* transform, const Vector3& delta);
}

namespace ComponentAPI
{
    ENGINE_API GameObject* getOwner(Component* component);
    ENGINE_API const GameObject* getOwner(const Component* component);

    ENGINE_API bool isActive(const Component* component);
    ENGINE_API void setActive(Component* component, bool active);
}

namespace SceneAPI
{
    ENGINE_API int countGameObjectsByComponent(ComponentType componentType, bool onlyActive = true);
    ENGINE_API int findGameObjectsByComponent(ComponentType componentType, GameObject** outputList, int maxResults, bool onlyActive = true);

    ENGINE_API GameObject* getDefaultCameraGameObject();
    ENGINE_API void setDefaultCameraByGameObject(GameObject* gameObject);

    ENGINE_API GameObject* findGameObjectByTag(Tag tag, bool onlyActive = true);

    ENGINE_API void requestSceneChange(const char* sceneName);
}

namespace Time
{
    ENGINE_API float getDeltaTime();
}

namespace Input
{
    ENGINE_API bool isKeyDown(KeyCode key);

    // GamePad Test

    ENGINE_API bool isGamePadConnected(int player);
    ENGINE_API Vector2 getGamePadLeftStick(int player);
    ENGINE_API bool isGamePadAPressed(int player);
}

namespace Debug
{
    ENGINE_API void log(const char* message, ...);
    ENGINE_API void warn(const char* message, ...);
    ENGINE_API void error(const char* message, ...);
}

namespace NavigationAPI
{
    ENGINE_API bool hasNavMesh();
    ENGINE_API bool samplePosition(const Vector3& inputPosition, Vector3& outSampledPosition, const Vector3& searchExtents);
    ENGINE_API bool moveAlongSurface(const Vector3& startPosition, const Vector3& targetPosition, Vector3& outResultPosition, const Vector3& searchExtents);
    ENGINE_API int findStraightPath(const Vector3& startPosition, const Vector3& endPosition, Vector3* outputPoints, int maxPoints, const Vector3& searchExtents);
    ENGINE_API bool canReachTarget(const Vector3& startPosition, const Vector3& endPosition, const Vector3& searchExtents);
    ENGINE_API float getPathLength(const Vector3* pathPoints, int pointCount);
    ENGINE_API bool findRandomReachablePointAround(const Vector3& centerPosition, float radius, Vector3& outPoint, const Vector3& searchExtents, int maxAttempts);
}