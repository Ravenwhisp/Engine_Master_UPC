#include "Globals.h"
#include "EngineAPI.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleTime.h"
#include "ModuleScene.h"
#include "Keyboard.h"
#include "ScriptFactory.h"

#include "GameObject.h"
#include "Transform.h"
#include "Component.h"

#include "CameraComponent.h"

void registerScript(const char* scriptName, ScriptCreator creator)
{
    ScriptFactory::registerScript(scriptName, creator);
}

namespace Input 
{
    bool isKeyDown(int key)
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

        return input->isKeyDown(static_cast<Keyboard::Keys>(key));
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

namespace GameObjectAPI
{
    Vector3 getPosition(const GameObject* gameObject)
    {
        return gameObject->GetTransform()->getPosition();
    }

    void setPosition(GameObject* gameObject, const Vector3& newPosition)
    {
        gameObject->GetTransform()->setPosition(newPosition);
    }

    Vector3 getEulerDegrees(const GameObject* gameObject)
    {
        return gameObject->GetTransform()->getEulerDegrees();
    }

    void setRotationEuler(GameObject* gameObject, const Vector3& eulerDegrees)
    {
        gameObject->GetTransform()->setRotationEuler(eulerDegrees);
    }
}

namespace ComponentAPI
{
    GameObject* getOwner(const Component* component)
    {
        return component->getOwner();
    }
}

namespace Scene
{
    int countGameObjectsByComponent(ComponentType componentType, bool onlyActive)
    {
        if (!app || !app->getModuleScene())
        {
            return 0;
        }

        int count = 0;

        for (GameObject* gameObject : app->getModuleScene()->getAllGameObjects())
        {
            if (!gameObject)
            {
                continue;
            }

            if (onlyActive && !gameObject->GetActive())
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

        for (GameObject* gameObject : app->getModuleScene()->getAllGameObjects())
        {
            if (!gameObject)
            {
                continue;
            }

            if (onlyActive && !gameObject->GetActive())
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

        CameraComponent* defaultCamera = app->getModuleScene()->getDefaultCamera();

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

        app->getModuleScene()->setDefaultCamera(camera);
    }
}