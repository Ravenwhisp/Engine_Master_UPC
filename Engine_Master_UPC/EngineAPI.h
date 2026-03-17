#pragma once

#ifdef ENGINE_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

#include "ScriptTypes.h"
#include "SimpleMath.h"

using DirectX::SimpleMath::Vector3;

class GameObject;
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
    ENGINE_API Vector3 getPosition(const GameObject* gameObject);
    ENGINE_API void setPosition(GameObject* gameObject, const Vector3& newPosition);

    ENGINE_API Vector3 getEulerDegrees(const GameObject* gameObject);
    ENGINE_API void setRotationEuler(GameObject* gameObject, const Vector3& eulerDegrees);
}

namespace ComponentAPI
{
    ENGINE_API GameObject* getOwner(const Component* component);
}