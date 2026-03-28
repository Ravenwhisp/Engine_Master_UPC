#include "Globals.h"
#include "EngineAPI.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleTime.h"
#include "ModuleScene.h"
#include "ModuleNavigation.h"

#include "Scene.h"
#include "Keyboard.h"
#include "ScriptFactory.h"

#include "GameObject.h"
#include "Transform.h"
#include "Component.h"

#include "CameraComponent.h"

#include <DetourNavMeshQuery.h>

void registerScript(const char* scriptName, ScriptCreator creator)
{
    ScriptFactory::registerScript(scriptName, creator);
}

namespace GameObjectAPI
{
    Transform* getTransform(GameObject* gameObject) 
    {
        return gameObject->GetTransform();
    }

    const Transform* getTransform(const GameObject* gameObject)
    {
        return gameObject->GetTransform();
    }

    bool isActiveSelf(const GameObject* gameObject)
    {
        return gameObject->GetActive();
    }

    bool isActiveInHierarchy(const GameObject* gameObject) 
    {
        return gameObject->IsActiveInWindowHierarchy();
    }

    void setActive(GameObject* gameObject, bool active)
    {
        gameObject->SetActive(active);
    }

    const char* getName(const GameObject* gameObject)
    {
        return gameObject->GetName().c_str();
    }

    Tag getTag(const GameObject* gameObject)
    {
        return gameObject->GetTag();
    }

    void setTag(GameObject* gameObject, Tag tag)
    {
        gameObject->SetTag(tag);
    }
}

namespace TransformAPI
{
    Vector3 getPosition(const Transform* transform)
    {
        return transform->getPosition();
    }

    void setPosition(Transform* transform, const Vector3& newPosition)
    {
        transform->setPosition(newPosition);
    }

    Vector3 getScale(const Transform* transform) 
    {
        return transform->getScale();
    }

    void setScale(Transform* transform, const Vector3& newScale) 
    {
        transform->setScale(newScale);
    }

    Vector3 getEulerDegrees(const Transform* transform)
    {
        return transform->getEulerDegrees();
    }

    void setRotationEuler(Transform* transform, const Vector3& eulerDegrees)
    {
        transform->setRotationEuler(eulerDegrees);
    }

    Vector3 getForward(const Transform* transform)
    {
        return transform->getForward();
    }

    Vector3 getRight(const Transform* transform)
    {
        return transform->getRight();
    }

    Vector3 getUp(const Transform* transform)
    {
        return transform->getUp();
    }

    void translate(Transform* transform, const Vector3& delta)
    {
        transform->setPosition(transform->getPosition() + delta);
    }
}

namespace ComponentAPI
{
    GameObject* getOwner(Component* component)
    {
        return component->getOwner();
    }

    const GameObject* getOwner(const Component* component)
    {
        return component->getOwner();
    }

    bool isActive(const Component* component)
    {
        return component->isActive();
    }

    void setActive(Component* component, bool active)
    {
        component->setActive(active);
    }
}

namespace SceneAPI
{
    int countGameObjectsByComponent(ComponentType componentType, bool onlyActive)
    {
        if (!app || !app->getModuleScene())
        {
            return 0;
        }

        int count = 0;

        for (GameObject* gameObject : app->getModuleScene()->getScene()->getAllGameObjects())
        {
            if (onlyActive && !gameObject->IsActiveInWindowHierarchy())
            {
                continue;
            }

            Component* component = nullptr;

            if (componentType == ComponentType::TRANSFORM)
            {
                component = gameObject->GetTransform();
            }
            else
            {
                component = gameObject->GetComponent(componentType);
            }

            if (!component)
            {
                continue;
            }

            if (onlyActive && !component->isActive())
            {
                continue;
            }

            ++count;
        }

        return count;
    }

    int findGameObjectsByComponent(ComponentType componentType, GameObject** outputList, int maxResults, bool onlyActive)
    {
        if (!app || !app->getModuleScene() || !outputList || maxResults <= 0)
        {
            return 0;
        }

        int count = 0;

        for (GameObject* gameObject : app->getModuleScene()->getScene()->getAllGameObjects())
        {
            if (onlyActive && !gameObject->IsActiveInWindowHierarchy())
            {
                continue;
            }

            Component* component = nullptr;

            if (componentType == ComponentType::TRANSFORM)
            {
                component = gameObject->GetTransform();
            }
            else
            {
                component = gameObject->GetComponent(componentType);
            }

            if (!component)
            {
                continue;
            }

            if (onlyActive && !component->isActive())
            {
                continue;
            }

            if (count >= maxResults)
            {
                break;
            }

            outputList[count] = gameObject;
            ++count;
        }

        return count;
    }

    GameObject* getDefaultCameraGameObject()
    {
        if (!app || !app->getModuleScene())
        {
            return nullptr;
        }

        CameraComponent* defaultCamera = app->getModuleScene()->getScene()->getDefaultCamera();

        return defaultCamera->getOwner();
    }

    void setDefaultCameraByGameObject(GameObject* gameObject)
    {
        if (!app || !app->getModuleScene() || !gameObject)
        {
            return;
        }

        CameraComponent* camera = gameObject->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
        if (!camera)
        {
            return;
        }

        app->getModuleScene()->getScene()->setDefaultCamera(camera);
    }

    GameObject* findGameObjectByTag(Tag tag, bool onlyActive)
    {
        if (!app || !app->getModuleScene())
        {
            return nullptr;
        }

        for (GameObject* gameObject : app->getModuleScene()->getScene()->getAllGameObjects())
        {
            if (onlyActive && !gameObject->IsActiveInWindowHierarchy())
            {
                continue;
            }

            if (gameObject->GetTag() != tag)
            {
                continue;
            }

            return gameObject;
        }

        return nullptr;
    }

    void requestSceneChange(const char* sceneName)
    {
        if (!app || !app->getModuleScene() || !sceneName || sceneName[0] == '\0')
        {
            return;
        }

        app->getModuleScene()->requestSceneChange(sceneName);
    }
}

namespace Input
{
    Keyboard::Keys toKeyboardKey(KeyCode key)
    {
        switch (key)
        {
        case KeyCode::W:          return Keyboard::Keys::W;
        case KeyCode::A:          return Keyboard::Keys::A;
        case KeyCode::S:          return Keyboard::Keys::S;
        case KeyCode::D:          return Keyboard::Keys::D;
        case KeyCode::Q:          return Keyboard::Keys::Q;
        case KeyCode::E:          return Keyboard::Keys::E;
        case KeyCode::I:          return Keyboard::Keys::I;
        case KeyCode::J:          return Keyboard::Keys::J;
        case KeyCode::K:          return Keyboard::Keys::K;
        case KeyCode::L:          return Keyboard::Keys::L;
        case KeyCode::U:          return Keyboard::Keys::U;
        case KeyCode::O:          return Keyboard::Keys::O;
        case KeyCode::T:          return Keyboard::Keys::T;
        case KeyCode::LeftShift:  return Keyboard::Keys::LeftShift;
        case KeyCode::RightShift: return Keyboard::Keys::RightShift;
        case KeyCode::Space:      return Keyboard::Keys::Space;
        case KeyCode::Escape:     return Keyboard::Keys::Escape;
        case KeyCode::Enter:      return Keyboard::Keys::Enter;
        case KeyCode::Up:         return Keyboard::Keys::Up;
        case KeyCode::Down:       return Keyboard::Keys::Down;
        case KeyCode::Left:       return Keyboard::Keys::Left;
        case KeyCode::Right:      return Keyboard::Keys::Right;
        case KeyCode::None:
        default:
            return Keyboard::Keys::None;
        }
    }

    bool isKeyDown(KeyCode key)
    {
        if (!app)
        {
            return false;
        }

        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        return input->isKeyDown(toKeyboardKey(key));
    }

    //GamePad Test

    bool isGamePadConnected(int player)
    {
        if (!app)
        {
            return false;
        }

        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        return input->isGamePadConnected(player);
    }

    Vector2 getGamePadLeftStick(int player)
    {
        if (!app)
        {
            return Vector2(0.0f, 0.0f);
        }

        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return Vector2(0.0f, 0.0f);
        }

        return input->getLeftStick(player);
    }

    bool isGamePadAPressed(int player)
    {
        if (!app)
        {
            return false;
        }

        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        return input->isGamePadAPressed(player);
    }
}

namespace Time
{
    float getDeltaTime()
    {
        if (!app)
        {
            return 0.0f;
        }

        if (!app->getModuleTime())
        {
            return 0.0f;
        }

        return app->getModuleTime()->deltaTime();
    }
}

namespace Debug
{
    void log(const char* message, ...)
    {
        if (!message)
        {
            return;
        }

        char buffer[1024];

        va_list args;
        va_start(args, message);
        vsnprintf(buffer, sizeof(buffer), message, args);
        va_end(args);

        DEBUG_LOG("%s", buffer);
    }

    void warn(const char* message, ...)
    {
        if (!message)
        {
            return;
        }

        char buffer[1024];

        va_list args;
        va_start(args, message);
        vsnprintf(buffer, sizeof(buffer), message, args);
        va_end(args);

        DEBUG_WARN("%s", buffer);
    }

    void error(const char* message, ...)
    {
        if (!message)
        {
            return;
        }

        char buffer[1024];

        va_list args;
        va_start(args, message);
        vsnprintf(buffer, sizeof(buffer), message, args);
        va_end(args);

        DEBUG_ERROR("%s", buffer);
    }
}

namespace NavigationAPI
{
    bool hasNavMesh()
    {
        ModuleNavigation* navigation = app->getModuleNavigation();
        if (!navigation)
        {
            return false;
        }

        return navigation->hasNavMesh();
    }

    bool samplePosition(const Vector3& inputPosition, Vector3& outSampledPosition, const Vector3& searchExtents)
    {
        ModuleNavigation* navigation = app->getModuleNavigation();
        if (!navigation || !navigation->hasNavMesh())
        {
            return false;
        }

        dtNavMeshQuery* query = navigation->getNavQuery();
        if (!query)
        {
            return false;
        }

        dtQueryFilter filter;
        filter.setIncludeFlags(0xFFFF);
        filter.setExcludeFlags(0);

        const float position[3] = { inputPosition.x, inputPosition.y, inputPosition.z };
        const float extents[3] = { searchExtents.x, searchExtents.y, searchExtents.z };

        dtPolyRef polyRef = 0;
        float nearest[3] = {};

        const dtStatus status = query->findNearestPoly(position, extents, &filter, &polyRef, nearest);
        if (dtStatusFailed(status) || !polyRef)
        {
            return false;
        }

        float height = nearest[1];
        if (dtStatusFailed(query->getPolyHeight(polyRef, nearest, &height)))
        {
            return false;
        }

        outSampledPosition = Vector3(nearest[0], height, nearest[2]);
        return true;
    }

    bool moveAlongSurface(const Vector3& startPosition, const Vector3& targetPosition, Vector3& outResultPosition, const Vector3& searchExtents)
    {
        ModuleNavigation* navigation = app->getModuleNavigation();
        if (!navigation || !navigation->hasNavMesh())
        {
            return false;
        }

        dtNavMeshQuery* query = navigation->getNavQuery();
        if (!query)
        {
            return false;
        }

        dtQueryFilter filter;
        filter.setIncludeFlags(0xFFFF);
        filter.setExcludeFlags(0);

        const float start[3] = { startPosition.x, startPosition.y, startPosition.z };
        const float end[3] = { targetPosition.x, targetPosition.y, targetPosition.z };
        const float extents[3] = { searchExtents.x, searchExtents.y, searchExtents.z };

        dtPolyRef startRef = 0;
        float startNearest[3] = {};

        const dtStatus nearestStatus = query->findNearestPoly(start, extents, &filter, &startRef, startNearest);
        if (dtStatusFailed(nearestStatus) || !startRef)
        {
            return false;
        }

        dtPolyRef visited[64];
        int visitedCount = 0;
        float result[3] = {};

        const dtStatus moveStatus = query->moveAlongSurface(startRef, startNearest, end, &filter, result, visited, &visitedCount, 64);

        if (dtStatusFailed(moveStatus))
        {
            return false;
        }

        dtPolyRef lastRef = (visitedCount > 0) ? visited[visitedCount - 1] : startRef;

        float height = result[1];
        if (dtStatusFailed(query->getPolyHeight(lastRef, result, &height)))
        {
            return false;
        }

        outResultPosition = Vector3(result[0], height, result[2]);
        return true;
    }

    int findStraightPath(const Vector3& startPosition, const Vector3& endPosition, Vector3* outputPoints, int maxPoints, const Vector3& searchExtents)
    {
        if (!outputPoints || maxPoints <= 0)
        {
            return 0;
        }

        ModuleNavigation* navigation = app->getModuleNavigation();
        if (!navigation || !navigation->hasNavMesh())
        {
            return 0;
        }

        std::vector<Vector3> path;
        if (!navigation->findStraightPath(startPosition, endPosition, path, searchExtents))
        {
            return 0;
        }

        const int count = (int)path.size() < maxPoints ? (int)path.size() : maxPoints;

        for (int i = 0; i < count; ++i)
        {
            outputPoints[i] = path[i];
        }

        return count;
    }

    bool canReachTarget(const Vector3& startPosition, const Vector3& endPosition, const Vector3& searchExtents)
    {
        ModuleNavigation* navigation = app->getModuleNavigation();
        if (!navigation || !navigation->hasNavMesh())
        {
            return false;
        }

        std::vector<Vector3> path;
        if (!navigation->findStraightPath(startPosition, endPosition, path, searchExtents))
        {
            return false;
        }

        return path.size() >= 2;
    }

    float getPathLength(const Vector3* pathPoints, int pointCount)
    {
        if (!pathPoints || pointCount < 2)
        {
            return 0.0f;
        }

        float totalLength = 0.0f;

        for (int i = 1; i < pointCount; ++i)
        {
            totalLength += (pathPoints[i] - pathPoints[i - 1]).Length();
        }

        return totalLength;
    }

    bool findRandomReachablePointAround(const Vector3& centerPosition, float radius, Vector3& outPoint, const Vector3& searchExtents, int maxAttempts)
    {
        ModuleNavigation* navigation = app->getModuleNavigation();
        if (!navigation || !navigation->hasNavMesh())
        {
            return false;
        }

        if (radius <= 0.0f || maxAttempts <= 0)
        {
            return false;
        }

        const float twoPi = 6.28318530717958647692f;

        for (int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            const float angle = ((float)std::rand() / (float)RAND_MAX) * twoPi;

            const float t = (float)std::rand() / (float)RAND_MAX;
            const float distance = std::sqrt(t) * radius;

            const Vector3 candidatePosition(centerPosition.x + std::cos(angle) * distance, centerPosition.y, centerPosition.z + std::sin(angle) * distance);

            Vector3 sampledPosition;
            if (!samplePosition(candidatePosition, sampledPosition, searchExtents))
            {
                continue;
            }

            if (!canReachTarget(centerPosition, sampledPosition, searchExtents))
            {
                continue;
            }

            outPoint = sampledPosition;
            return true;
        }

        return false;
    }
}