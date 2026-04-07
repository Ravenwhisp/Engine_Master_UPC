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
class Script;
class AnimationComponent;

ENGINE_API void registerScript(const char* scriptName, ScriptCreator creator);

namespace GameObjectAPI 
{
    ENGINE_API Transform* getTransform(GameObject* gameObject);
    ENGINE_API const Transform* getTransform(const GameObject* gameObject);

    ENGINE_API Script* getScript(GameObject* gameObject, const char* scriptName);
    ENGINE_API const Script* getScript(const GameObject* gameObject, const char* scriptName);

    ENGINE_API bool isActiveSelf(const GameObject* gameObject);
    ENGINE_API bool isActiveInHierarchy(const GameObject* gameObject);
    ENGINE_API void setActive(GameObject* gameObject, bool active);

    ENGINE_API const char* getName(const GameObject* gameObject);

    ENGINE_API Tag getTag(const GameObject* gameObject);
    ENGINE_API void setTag(GameObject* gameObject, Tag tag);

    ENGINE_API GameObject* createGameObject(const char* name, GameObject* parentObject = nullptr);
    ENGINE_API void removeGameObject(GameObject* gameObject);

    //ENGINE_API GameObject* instantiate(GameObject* gameObject, const Vector3& position, const Vector3& rotationEuler, GameObject* parentObject = nullptr);
    ENGINE_API GameObject* instantiatePrefab(const char* path, const Vector3& position, const Vector3& rotationEuler, GameObject* parentObject = nullptr);
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

    ENGINE_API Transform* getParent(Transform* transform);
    ENGINE_API const Transform* getParent(const Transform* transform);

    ENGINE_API Transform* findChildByName(Transform* transform, const char* childName);
    ENGINE_API const Transform* findChildByName(const Transform* transform, const char* childName);
}

namespace ComponentAPI
{
    ENGINE_API GameObject* getOwner(Component* component);
    ENGINE_API const GameObject* getOwner(const Component* component);

    ENGINE_API bool isActive(const Component* component);
    ENGINE_API void setActive(Component* component, bool active);
}

namespace AnimationAPI
{
    ENGINE_API AnimationComponent* getAnimationComponent(GameObject* gameObject);
    ENGINE_API const AnimationComponent* getAnimationComponent(const GameObject* gameObject);

    ENGINE_API bool hasStateMachine(const AnimationComponent* animation);
    ENGINE_API bool hasActiveState(const AnimationComponent* animation);
    ENGINE_API const char* getActiveStateName(const AnimationComponent* animation);

    ENGINE_API bool playState(AnimationComponent* animation, const char* stateName, float transitionTimeSeconds = 0.0f);
    ENGINE_API bool playDefaultState(AnimationComponent* animation, float transitionTimeSeconds = 0.0f);
    ENGINE_API bool sendTrigger(AnimationComponent* animation, const char* triggerName);

    ENGINE_API void play(AnimationComponent* animation);
    ENGINE_API void pause(AnimationComponent* animation);
    ENGINE_API void stop(AnimationComponent* animation);
    ENGINE_API bool isPlaying(const AnimationComponent* animation);

    ENGINE_API float getPlaybackTime(const AnimationComponent* animation);
    ENGINE_API void setPlaybackTime(AnimationComponent* animation, float seconds);
    ENGINE_API float getPlaybackDuration(const AnimationComponent* animation);

    ENGINE_API float getSpeedMultiplier(const AnimationComponent* animation);
    ENGINE_API void setSpeedMultiplier(AnimationComponent* animation, float speedMultiplier);
}

namespace SceneAPI
{
    ENGINE_API std::vector<GameObject*> findAllGameObjectsByComponent(ComponentType componentType, bool onlyActive = true);
    ENGINE_API std::vector<GameObject*> findAllGameObjectsByTag(Tag tag, bool onlyActive = true);

    ENGINE_API GameObject* getDefaultCameraGameObject();
    ENGINE_API void setDefaultCameraByGameObject(GameObject* gameObject);

    ENGINE_API void requestSceneChange(const char* sceneName);
}

namespace Time
{
    ENGINE_API float getDeltaTime();
}

namespace Input
{
    // Normally we should not need this but I let here just in case
    ENGINE_API bool isKeyDown(KeyCode key);

    ENGINE_API Vector2 getMoveAxis(int player = 0);
    ENGINE_API Vector2 getLookAxis(int player = 0);

    ENGINE_API bool isLeftStickPressed(int player = 0);
    ENGINE_API bool isRightStickPressed(int player = 0);

    ENGINE_API bool isLeftStickJustPressed(int player = 0);
    ENGINE_API bool isRightStickJustPressed(int player = 0);

    ENGINE_API bool isLeftStickReleased(int player = 0);
    ENGINE_API bool isRightStickReleased(int player = 0);

    ENGINE_API bool isFaceButtonBottomPressed(int player = 0);
    ENGINE_API bool isFaceButtonRightPressed(int player = 0);
    ENGINE_API bool isFaceButtonLeftPressed(int player = 0);
    ENGINE_API bool isFaceButtonTopPressed(int player = 0);

    ENGINE_API bool isFaceButtonBottomJustPressed(int player = 0);
    ENGINE_API bool isFaceButtonRightJustPressed(int player = 0);
    ENGINE_API bool isFaceButtonLeftJustPressed(int player = 0);
    ENGINE_API bool isFaceButtonTopJustPressed(int player = 0);

    ENGINE_API bool isFaceButtonBottomReleased(int player = 0);
    ENGINE_API bool isFaceButtonRightReleased(int player = 0);
    ENGINE_API bool isFaceButtonLeftReleased(int player = 0);
    ENGINE_API bool isFaceButtonTopReleased(int player = 0);

    ENGINE_API bool isLeftShoulderPressed(int player = 0);
    ENGINE_API bool isRightShoulderPressed(int player = 0);

    ENGINE_API bool isLeftShoulderJustPressed(int player = 0);
    ENGINE_API bool isRightShoulderJustPressed(int player = 0);

    ENGINE_API bool isLeftShoulderReleased(int player = 0);
    ENGINE_API bool isRightShoulderReleased(int player = 0);

    ENGINE_API bool isLeftTriggerPressed(int player = 0);
    ENGINE_API bool isRightTriggerPressed(int player = 0);

    ENGINE_API bool isLeftTriggerJustPressed(int player = 0);
    ENGINE_API bool isRightTriggerJustPressed(int player = 0);

    ENGINE_API bool isLeftTriggerReleased(int player = 0);
    ENGINE_API bool isRightTriggerReleased(int player = 0);

    ENGINE_API bool isPausePressed(int player = 0);
    ENGINE_API bool isPauseJustPressed(int player = 0);
    ENGINE_API bool isPauseReleased(int player = 0);

    ENGINE_API void setPlayerKeyboard(int player);
    ENGINE_API void setPlayerGamepad(int player, int gamepadIndex);
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

namespace DebugDrawAPI
{
    ENGINE_API void drawPoint(const Vector3& pos, const Vector3& color, float size = 1.0f, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawLine(const Vector3& from, const Vector3& to, const Vector3& color, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawScreenText(const char* str, const Vector3& pos, const Vector3& color, float scaling = 1.0f, int durationMillis = 0);
    ENGINE_API void drawProjectedText(const char* str, const Vector3& pos, const Vector3& color, const Matrix& vpMatrix, int sx, int sy, int sw, int sh, float scaling = 1.0f, int durationMillis = 0);
    ENGINE_API void drawAxisTriad(const Matrix& transform, float size, float length, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawArrow(const Vector3& from, const Vector3& to, const Vector3& color, float size = 1.0f, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawCross(const Vector3& center, float length, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawCircle(const Vector3& center, const Vector3& planeNormal, const Vector3& color, float radius, float numSteps = 16.0f, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawPlane(const Vector3& center, const Vector3& planeNormal, const Vector3& planeColor, const Vector3& normalVecColor, float planeScale, float normalVecScale, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawSphere(const Vector3& center, const Vector3& color, float radius, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawCone(const Vector3& apex, const Vector3& dir, const Vector3& color, float baseRadius, float apexRadius = 0.0f, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawBox(const Vector3& center, const Vector3& color, float width, float height, float depth, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawAABB(const Vector3& mins, const Vector3& maxs, const Vector3& color, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawFrustum(const Matrix& invClipMatrix, const Vector3& color, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawVertexNormal(const Vector3& origin, const Vector3& normal, float length = 1.0f, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawTangentBasis(const Vector3& origin, const Vector3& normal, const Vector3& tangent, const Vector3& bitangent, float lengths = 1.0f, int durationMillis = 0, bool depthEnabled = true);
    ENGINE_API void drawXZSquareGrid(float mins, float maxs, float y, float step, const Vector3& color, int durationMillis = 0, bool depthEnabled = true);
}