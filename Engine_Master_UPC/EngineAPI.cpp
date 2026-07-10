#include "Globals.h"
#include "EngineAPI.h"

#include "Application.h"
#include "ModuleInput.h"
#include "ModuleTime.h"
#include "ModuleScene.h"
#include "ModuleNavigation.h"
#include "ModuleEditor.h"
#include "ModuleAssets.h"
#include "ModuleMusic.h"

#include "PrefabManager.h"
#include "Quadtree.h"

#include "Scene.h"
#include "Keyboard.h"
#include "GenericTypeFactory.h"

#include "GameObject.h"
#include "Transform.h"
#include "Component.h"
#include "ScriptComponent.h"
#include "Script.h"
#include "AnimationComponent.h"
#include "UISlider.h"
#include "UISheet.h"
#include "Transform2D.h"
#include "MeshRenderer.h"
#include "BoundingBox.h"
#include "ParticleSystemComponent.h"
#include "ComponentSoundSource.h"
#include "NavRuntimeBlockerComponent.h"
#include "PlayerRenderBufferComponent.h"
#include "TrailComponent.h"

#include "CameraComponent.h"

#include "HapticEffectDefinition.h"
#include "HapticEffectLibrary.h"
#include "ModuleHaptics.h"

#include "DeviceType.h"
#include "PlayerBinding.h"

#include "HierarchyUtils.h"

#include <DetourNavMeshQuery.h>

void registerScript(const char* scriptName, ScriptFactory::Creator creator)
{
	ScriptFactory::registerType(scriptName, scriptName, creator);
}

void registerDataContainer(const char* name, const char* displayName, DataContainerFactory::Creator creator)
{
	DataContainerFactory::registerType(name, displayName, creator);
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

    Component* getComponent(GameObject* gameObject, ComponentType type)
    {
        if (gameObject == nullptr)
        {
            return nullptr;
        }
        return gameObject->GetComponent(type);
    }

    const Component* getComponent(const GameObject* gameObject, ComponentType type)
    {
        if (gameObject == nullptr)
        {
            return nullptr;
        }
        return gameObject->GetComponent(type);
    }

    Script* GameObjectAPI::getScript(GameObject* gameObject, const char* scriptName)
    {
        if (gameObject == nullptr || scriptName == nullptr)
        {
            return nullptr;
        }

        const std::vector<Component*> components = gameObject->GetAllComponents();

        for (Component* component : components)
        {
            if (component == nullptr || component->getType() != ComponentType::SCRIPT)
            {
                continue;
            }

            ScriptComponent* scriptComponent = static_cast<ScriptComponent*>(component);

            if (scriptComponent->getScriptName() == scriptName)
            {
                return scriptComponent->getScript();
            }
        }

        return nullptr;
    }

    const Script* GameObjectAPI::getScript(const GameObject* gameObject, const char* scriptName)
    {
        if (gameObject == nullptr || scriptName == nullptr)
        {
            return nullptr;
        }

        const std::vector<Component*> components = gameObject->GetAllComponents();

        for (Component* component : components)
        {
            if (component == nullptr || component->getType() != ComponentType::SCRIPT)
            {
                continue;
            }

            const ScriptComponent* scriptComponent = static_cast<const ScriptComponent*>(component);

            if (scriptComponent->getScriptName() == scriptName)
            {
                return scriptComponent->getScript();
            }
        }

        return nullptr;
    }

    int GameObjectAPI::getScriptCount(const GameObject* gameObject)
    {
        if (gameObject == nullptr)
        {
            return 0;
        }

        int count = 0;

        const std::vector<Component*> components = gameObject->GetAllComponents();

        for (Component* component : components)
        {
            if (component == nullptr || component->getType() != ComponentType::SCRIPT)
            {
                continue;
            }

            ++count;
        }

        return count;
    }

    Script* GameObjectAPI::getScriptByIndex(GameObject* gameObject, int index)
    {
        if (gameObject == nullptr || index < 0)
        {
            return nullptr;
        }

        int currentIndex = 0;

        const std::vector<Component*> components = gameObject->GetAllComponents();

        for (Component* component : components)
        {
            if (component == nullptr || component->getType() != ComponentType::SCRIPT)
            {
                continue;
            }

            ScriptComponent* scriptComponent = static_cast<ScriptComponent*>(component);

            if (currentIndex == index)
            {
                return scriptComponent->getScript();
            }

            ++currentIndex;
        }

        return nullptr;
    }

    const Script* GameObjectAPI::getScriptByIndex(const GameObject* gameObject, int index)
    {
        if (gameObject == nullptr || index < 0)
        {
            return nullptr;
        }

        int currentIndex = 0;

        const std::vector<Component*> components = gameObject->GetAllComponents();

        for (Component* component : components)
        {
            if (component == nullptr || component->getType() != ComponentType::SCRIPT)
            {
                continue;
            }

            const ScriptComponent* scriptComponent = static_cast<const ScriptComponent*>(component);

            if (currentIndex == index)
            {
                return scriptComponent->getScript();
            }

            ++currentIndex;
        }

        return nullptr;
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
    
    GameObject* createGameObject(const char* name, GameObject* parentObject)
    {
        Scene* currentScene = app->getModuleScene()->getScene();

        GameObject* createdObject = currentScene->createGameObject();
        createdObject->SetName(name);

        if (parentObject) 
        {
            HierarchyUtils::reparent(currentScene, createdObject, parentObject);
        }

        return createdObject;
    }

    void removeGameObject(GameObject* gameObject)
    {
        ModuleEditor* editorModule = app->getModuleEditor();

        if (editorModule->getSelectedGameObject() == gameObject)
        {
            editorModule->setSelectedGameObject(nullptr);
        }

        Scene* currentScene = app->getModuleScene()->getScene();
        currentScene->markGameObjectForRemoval(gameObject->GetID());
    }

    /*
    ENGINE_API GameObject* instantiate(GameObject* gameObject, const Vector3& position, const Vector3& rotationEuler, GameObject* parentObject)
    {
        return nullptr;
    }
    */

    ENGINE_API GameObject* instantiatePrefab(const AssetReference& prefabRef, const Vector3& position, const Vector3& rotationEuler, GameObject* parentObject)
    {
        Scene* currentScene = app->getModuleScene()->getScene();
        
        GameObject* prefabInstance = app->getModuleAssets()->getPrefabManager()->spawnPrefab(prefabRef, currentScene);

        if (!prefabInstance) return nullptr;

        Transform* instanceTransform = prefabInstance->GetTransform();
        instanceTransform->setPosition(position);
        instanceTransform->setRotationEuler(rotationEuler);

        if (parentObject) 
        {
            HierarchyUtils::reparent(currentScene, prefabInstance, parentObject);
        }

        return prefabInstance;
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

    Vector3 getGlobalPosition(const Transform* transform)
    {
        if (transform == nullptr)
        {
            return Vector3::Zero;
        }

        return transform->getGlobalMatrix().Translation();
    }

    void setGlobalPosition(Transform* transform, const Vector3& worldPosition)
    {
        if (transform == nullptr)
        {
            return;
        }

        Matrix globalMatrix = transform->getGlobalMatrix();
        globalMatrix.Translation(worldPosition);

        transform->setFromGlobalMatrix(globalMatrix);
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

    Vector3 getGlobalEulerDegrees(const Transform* transform)
    {
        if (transform == nullptr)
        {
            return Vector3::Zero;
        }

        const Matrix& globalMatrix = transform->getGlobalMatrix();
        Quaternion globalRotation = Quaternion::CreateFromRotationMatrix(globalMatrix);

        Vector3 eulerRadians = globalRotation.ToEuler();
        const float radToDeg = 57.2957795f;

        return Vector3(eulerRadians.x * radToDeg, eulerRadians.y * radToDeg, eulerRadians.z * radToDeg);
    }

    void setGlobalRotationEuler(Transform* transform, const Vector3& eulerDegrees)
    {
        if (transform == nullptr)
        {
            return;
        }

        Matrix currentGlobalMatrix = transform->getGlobalMatrix();

        Vector3 globalScale;
        Quaternion currentGlobalRotation;
        Vector3 globalPosition;

        if (!currentGlobalMatrix.Decompose(globalScale, currentGlobalRotation, globalPosition))
        {
            return;
        }

        Quaternion desiredGlobalRotation = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(eulerDegrees.y), XMConvertToRadians(eulerDegrees.x), XMConvertToRadians(eulerDegrees.z));
        desiredGlobalRotation.Normalize();

        Matrix newGlobalMatrix = Matrix::CreateScale(globalScale) * Matrix::CreateFromQuaternion(desiredGlobalRotation) * Matrix::CreateTranslation(globalPosition);

        transform->setFromGlobalMatrix(newGlobalMatrix);
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

    void translateGlobal(Transform* transform, const Vector3& delta)
    {
        if (transform == nullptr)
        {
            return;
        }

        const Vector3 currentGlobalPosition = getGlobalPosition(transform);
        setGlobalPosition(transform, currentGlobalPosition + delta);
    }

    void lookAt(Transform* transform, const Vector3& targetWorldPosition)
    {
        if (transform == nullptr)
        {
            return;
        }

        Matrix currentGlobalMatrix = transform->getGlobalMatrix();

        Vector3 globalScale;
        Quaternion currentGlobalRotation;
        Vector3 globalPosition;

        if (!currentGlobalMatrix.Decompose(globalScale, currentGlobalRotation, globalPosition))
        {
            return;
        }

        Vector3 forward = targetWorldPosition - globalPosition;
        if (forward.LengthSquared() <= 0.0001f)
        {
            return;
        }
        forward.Normalize();

        Vector3 up = Vector3::Up;

        if (fabsf(forward.Dot(up)) > 0.999f)
        {
            up = Vector3::Right;
        }

        Vector3 right = up.Cross(forward);
        right.Normalize();

        Vector3 correctedUp = forward.Cross(right);
        correctedUp.Normalize();

        Matrix rotationMatrix = Matrix::Identity;
        rotationMatrix._11 = right.x;
        rotationMatrix._12 = right.y;
        rotationMatrix._13 = right.z;

        rotationMatrix._21 = correctedUp.x;
        rotationMatrix._22 = correctedUp.y;
        rotationMatrix._23 = correctedUp.z;

        rotationMatrix._31 = forward.x;
        rotationMatrix._32 = forward.y;
        rotationMatrix._33 = forward.z;

        Quaternion desiredGlobalRotation = Quaternion::CreateFromRotationMatrix(rotationMatrix);
        desiredGlobalRotation.Normalize();

        Matrix newGlobalMatrix = Matrix::CreateScale(globalScale) * Matrix::CreateFromQuaternion(desiredGlobalRotation) * Matrix::CreateTranslation(globalPosition);

        transform->setFromGlobalMatrix(newGlobalMatrix);
    }

    Transform* TransformAPI::getParent(Transform* transform)
    {
        return transform->getRoot();
    }

    const Transform* TransformAPI::getParent(const Transform* transform)
    {
        return transform->getRoot();
    }

    int TransformAPI::getChildCount(const Transform* transform)
    {
        if (transform == nullptr)
        {
            return 0;
        }

        return static_cast<int>(transform->getAllChildren().size());
    }

    Transform* TransformAPI::getChild(Transform* transform, int index)
    {
        if (transform == nullptr)
        {
            return nullptr;
        }

        const std::vector<GameObject*>& children = transform->getAllChildren();

        if (index < 0 || index >= static_cast<int>(children.size()))
        {
            return nullptr;
        }

        GameObject* child = children[index];
        if (child == nullptr)
        {
            return nullptr;
        }

        return child->GetTransform();
    }

    const Transform* TransformAPI::getChild(const Transform* transform, int index)
    {
        if (transform == nullptr)
        {
            return nullptr;
        }

        const std::vector<GameObject*>& children = transform->getAllChildren();

        if (index < 0 || index >= static_cast<int>(children.size()))
        {
            return nullptr;
        }

        const GameObject* child = children[index];
        if (child == nullptr)
        {
            return nullptr;
        }

        return child->GetTransform();
    }

    Transform* TransformAPI::findChildByName(Transform* transform, const char* childName)
    {
        if (transform == nullptr || childName == nullptr)
        {
            return nullptr;
        }

        const std::vector<GameObject*>& children = transform->getAllChildren();

        for (GameObject* child : children)
        {
            if (child == nullptr)
            {
                continue;
            }

            if (child->GetName() == childName)
            {
                return child->GetTransform();
            }
        }

        return nullptr;
    }

    const Transform* TransformAPI::findChildByName(const Transform* transform, const char* childName)
    {
        if (transform == nullptr || childName == nullptr)
        {
            return nullptr;
        }

        const std::vector<GameObject*>& children = transform->getAllChildren();

        for (GameObject* child : children)
        {
            if (child == nullptr)
            {
                continue;
            }

            if (child->GetName() == childName)
            {
                return child->GetTransform();
            }
        }

        return nullptr;
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

namespace AnimationAPI
{
    AnimationComponent* getAnimationComponent(GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<AnimationComponent>(ComponentType::ANIMATION);
    }

    const AnimationComponent* getAnimationComponent(const GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<AnimationComponent>(ComponentType::ANIMATION);
    }

    bool hasStateMachine(const AnimationComponent* animation)
    {
        return animation ? animation->hasStateMachine() : false;
    }

    bool hasActiveState(const AnimationComponent* animation)
    {
        return animation ? animation->hasActiveState() : false;
    }

    const char* getActiveStateName(const AnimationComponent* animation)
    {
        if (!animation)
        {
            return "";
        }

        return animation->getActiveStateName().c_str();
    }

    bool playState(AnimationComponent* animation, const char* stateName, float transitionTimeSeconds)
    {
        if (!animation || !stateName)
        {
            return false;
        }

        return animation->playState(stateName, transitionTimeSeconds);
    }

    bool playDefaultState(AnimationComponent* animation, float transitionTimeSeconds)
    {
        if (!animation)
        {
            return false;
        }

        return animation->playDefaultState(transitionTimeSeconds);
    }

    bool sendTrigger(AnimationComponent* animation, const char* triggerName)
    {
        if (!animation || !triggerName)
        {
            return false;
        }

        return animation->sendTrigger(triggerName);
    }

    void play(AnimationComponent* animation)
    {
        if (!animation)
        {
            return;
        }

        animation->play();
    }

    void pause(AnimationComponent* animation)
    {
        if (!animation)
        {
            return;
        }

        animation->pause();
    }

    void stop(AnimationComponent* animation)
    {
        if (!animation)
        {
            return;
        }

        animation->stop();
    }

    bool isPlaying(const AnimationComponent* animation)
    {
        return animation ? animation->isPlaying() : false;
    }

    float getPlaybackTime(const AnimationComponent* animation)
    {
        return animation ? animation->getPlaybackTime() : 0.0f;
    }

    void setPlaybackTime(AnimationComponent* animation, float seconds)
    {
        if (!animation)
        {
            return;
        }

        animation->setPlaybackTime(seconds);
    }

    float getPlaybackDuration(const AnimationComponent* animation)
    {
        return animation ? animation->getPlaybackDuration() : 0.0f;
    }

    float getSpeedMultiplier(const AnimationComponent* animation)
    {
        return animation ? animation->getSpeedMultiplier() : 0.0f;
    }

    void setSpeedMultiplier(AnimationComponent* animation, float speedMultiplier)
    {
        if (!animation)
        {
            return;
        }

        animation->setSpeedMultiplier(speedMultiplier);
    }

    bool playOverrideClip(AnimationComponent* animation, const char* clipName, float transitionTimeSeconds, bool loop)
    {
        if (!animation || !clipName)
        {
            return false;
        }

        return animation->playOverrideClip(clipName, transitionTimeSeconds, loop);
    }

    void clearOverrideClip(AnimationComponent* animation, float transitionTimeSeconds)
    {
        if (!animation)
        {
            return;
        }

        animation->clearOverrideClip(transitionTimeSeconds);
    }

    bool hasOverrideClip(const AnimationComponent* animation)
    {
        return animation ? animation->hasOverrideClip() : false;
    }

}

namespace ApplicationAPI
{
    void quit()
    {
        app->requestApplicationExit();
    }
}

namespace SceneAPI
{
    std::vector<GameObject*> findAllGameObjectsByComponent(ComponentType componentType, bool onlyActive)
    {
        std::vector<GameObject*> result;

        if (!app || !app->getModuleScene())
        {
            return result;
        }

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

            result.push_back(gameObject);
        }

        return result;
    }

    std::vector<GameObject*> findAllGameObjectsByTag(Tag tag, bool onlyActive)
    {
        std::vector<GameObject*> result;

        if (!app || !app->getModuleScene())
        {
            return result;
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

            result.push_back(gameObject);
        }

        return result;
    }

    std::vector<GameObject*> getAllGameObjectsInScene(bool onlyActive)
    {
        std::vector<GameObject*> result;
        if (!app || !app->getModuleScene())
        {
            return result;
        }
        for (GameObject* gameObject : app->getModuleScene()->getScene()->getAllGameObjects())
        {
            if (onlyActive && !gameObject->IsActiveInWindowHierarchy())
            {
                continue;
            }
            result.push_back(gameObject);
        }
		return result;
    }

    std::vector<GameObject*> getObjectsInCircularArea(const Vector2& center, const float radius, bool onlyActive, QuadtreeTarget target)
    {
        std::vector<GameObject*> candidates;

		auto* sceneModule = app->getModuleScene();

        if ((static_cast<uint8_t>(target) & static_cast<uint8_t>(QuadtreeTarget::Static)) != 0)
        {
            auto staticResult = sceneModule->getStaticQuadtree()->queryInArea(center, radius);
            candidates.insert(candidates.end(), staticResult.begin(), staticResult.end());
        }
        if ((static_cast<uint8_t>(target) & static_cast<uint8_t>(QuadtreeTarget::Dynamic)) != 0)
        {
            auto dynamicResult = sceneModule->getDynamicQuadtree()->queryInArea(center, radius);
            candidates.insert(candidates.end(), dynamicResult.begin(), dynamicResult.end());
        }

        std::vector<GameObject*> result;
        for (GameObject* candidate : candidates)
        {
            if (onlyActive && !candidate->IsActiveInWindowHierarchy())
            {
                continue;
            }

            auto* model = candidate->GetComponentAs<MeshRenderer>(ComponentType::MODEL);

            if (!model || !candidate->GetActive())
            {
                continue;
            }

            const Engine::BoundingBox bbox = model->getBoundingBox();

            Vector3 bMin = bbox.getMinInWorldSpace();
            Vector3 bMax = bbox.getMaxInWorldSpace();

            // Correct min and max problem
            float rMinX = std::min(bMin.x, bMax.x);
            float rMaxX = std::max(bMin.x, bMax.x);
            float rMinZ = std::min(bMin.z, bMax.z);
            float rMaxZ = std::max(bMin.z, bMax.z);

            float closestX = std::clamp(center.x, rMinX, rMaxX);
            float closestZ = std::clamp(center.y, rMinZ, rMaxZ);

            float diffX = center.x - closestX;
            float diffZ = center.y - closestZ;
            float distanceSquared = (diffX * diffX) + (diffZ * diffZ);

            if (distanceSquared <= radius * radius)
            {
				const Transform* transform = candidate->GetTransform();
                result.push_back((transform && transform->getRoot()) ? transform->getRoot()->getOwner() : candidate);
            }
        }

		return result;
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
    static Keyboard::Keys toKeyboardKey(KeyCode key)
    {
        switch (key)
        {
        case KeyCode::W:          return Keyboard::Keys::W;
        case KeyCode::A:          return Keyboard::Keys::A;
        case KeyCode::S:          return Keyboard::Keys::S;
        case KeyCode::D:          return Keyboard::Keys::D;
        case KeyCode::Q:          return Keyboard::Keys::Q;
        case KeyCode::E:          return Keyboard::Keys::E;
        case KeyCode::R:          return Keyboard::Keys::R;
        case KeyCode::I:          return Keyboard::Keys::I;
        case KeyCode::J:          return Keyboard::Keys::J;
        case KeyCode::K:          return Keyboard::Keys::K;
        case KeyCode::L:          return Keyboard::Keys::L;
        case KeyCode::U:          return Keyboard::Keys::U;
        case KeyCode::O:          return Keyboard::Keys::O;
        case KeyCode::T:          return Keyboard::Keys::T;
        case KeyCode::F:          return Keyboard::Keys::F;
        case KeyCode::LeftShift:  return Keyboard::Keys::LeftShift;
        case KeyCode::RightShift: return Keyboard::Keys::RightShift;
        case KeyCode::Space:      return Keyboard::Keys::Space;
        case KeyCode::Escape:     return Keyboard::Keys::Escape;
        case KeyCode::Enter:      return Keyboard::Keys::Enter;
        case KeyCode::Tab:        return Keyboard::Keys::Tab;
        case KeyCode::Up:         return Keyboard::Keys::Up;
        case KeyCode::Down:       return Keyboard::Keys::Down;
        case KeyCode::Left:       return Keyboard::Keys::Left;
        case KeyCode::Right:      return Keyboard::Keys::Right;
        case KeyCode::Num1:       return Keyboard::Keys::D1;
        case KeyCode::Num2:       return Keyboard::Keys::D2;
        case KeyCode::Num3:       return Keyboard::Keys::D3;
        case KeyCode::Num4:       return Keyboard::Keys::D4;
        case KeyCode::None:
        default:
            return Keyboard::Keys::None;
        }
    }

    enum class ButtonPhase
    {
        Pressed,
        JustPressed,
        Released
    };

    enum class FaceButton
    {
        Bottom,
        Right,
        Left,
        Top
    };

    enum class StickButton
    {
        Left,
        Right
    };

    enum class ShoulderButton
    {
        Left,
        Right
    };

    enum class TriggerButton
    {
        Left,
        Right
    };

    static bool queryKeyboardKey(KeyCode key, ButtonPhase phase)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        const Keyboard::Keys keyboardKey = toKeyboardKey(key);

        switch (phase)
        {
        case ButtonPhase::Pressed:
            return input->isKeyDown(keyboardKey);

        case ButtonPhase::JustPressed:
            return input->isKeyJustPressed(keyboardKey);

        case ButtonPhase::Released:
            return input->isKeyReleased(keyboardKey);

        default:
            return false;
        }
    }

    static bool queryRightMouseButton(ButtonPhase phase)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        switch (phase)
        {
        case ButtonPhase::Pressed:
            return input->isRightMouseHeld();

        case ButtonPhase::JustPressed:
            return input->isRightMousePressed();

        case ButtonPhase::Released:
            return input->isRightMouseReleased();

        default:
            return false;
        }
    }

    static bool queryLeftMouseButton(ButtonPhase phase)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        switch (phase)
        {
        case ButtonPhase::Pressed:
            return input->isLeftMouseHeld();

        case ButtonPhase::JustPressed:
            return input->isLeftMousePressed();

        case ButtonPhase::Released:
            return input->isLeftMouseReleased();

        default:
            return false;
        }
    }

    static bool queryGamepadButton(ModuleInput* input, int deviceIndex, SDL_GamepadButton button, ButtonPhase phase)
    {
        switch (button)
        {
        case SDL_GAMEPAD_BUTTON_SOUTH:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadAPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadAJustPressed(deviceIndex);
            return input->isGamePadAReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_EAST:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadBPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadBJustPressed(deviceIndex);
            return input->isGamePadBReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_WEST:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadXPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadXJustPressed(deviceIndex);
            return input->isGamePadXReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_NORTH:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadYPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadYJustPressed(deviceIndex);
            return input->isGamePadYReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadLeftShoulderPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadLeftShoulderJustPressed(deviceIndex);
            return input->isGamePadLeftShoulderReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadRightShoulderPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadRightShoulderJustPressed(deviceIndex);
            return input->isGamePadRightShoulderReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadDPadUpPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadDPadUpJustPressed(deviceIndex);
            return input->isGamePadDPadUpReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadDPadDownPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadDPadDownJustPressed(deviceIndex);
            return input->isGamePadDPadDownReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadDPadLeftPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadDPadLeftJustPressed(deviceIndex);
            return input->isGamePadDPadLeftReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadDPadRightPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadDPadRightJustPressed(deviceIndex);
            return input->isGamePadDPadRightReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_START:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadStartPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadStartJustPressed(deviceIndex);
            return input->isGamePadStartReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadLeftStickPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadLeftStickJustPressed(deviceIndex);
            return input->isGamePadLeftStickReleased(deviceIndex);

        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadRightStickPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadRightStickJustPressed(deviceIndex);
            return input->isGamePadRightStickReleased(deviceIndex);

        default:
            return false;
        }
    }

    static bool queryGamepadTrigger(ModuleInput* input, TriggerButton triggerButton, int deviceIndex, ButtonPhase phase)
    {
        switch (triggerButton)
        {
        case TriggerButton::Left:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadLeftTriggerPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadLeftTriggerJustPressed(deviceIndex);
            return input->isGamePadLeftTriggerReleased(deviceIndex);

        case TriggerButton::Right:
            if (phase == ButtonPhase::Pressed)     return input->isGamePadRightTriggerPressed(deviceIndex);
            if (phase == ButtonPhase::JustPressed) return input->isGamePadRightTriggerJustPressed(deviceIndex);
            return input->isGamePadRightTriggerReleased(deviceIndex);
        }

        return false;
    }

    static bool queryFaceButton(FaceButton faceButton, ButtonPhase phase, int player)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        const PlayerBinding binding = input->getPlayerBinding(player);

        switch (binding.deviceType)
        {
        case DeviceType::Keyboard:
            switch (faceButton)
            {
            case FaceButton::Bottom:
                return queryKeyboardKey(KeyCode::Space, phase);

            case FaceButton::Right:
                return queryKeyboardKey(KeyCode::R, phase);

            case FaceButton::Left:
                return queryKeyboardKey(KeyCode::T, phase);

            case FaceButton::Top:
                return queryKeyboardKey(KeyCode::Q, phase);
            }

            return false;

        case DeviceType::Gamepad:
            switch (faceButton)
            {
            case FaceButton::Bottom:
                return queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_SOUTH, phase);

            case FaceButton::Right:
                return queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_EAST, phase);

            case FaceButton::Left:
                return queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_WEST, phase);

            case FaceButton::Top:
                return queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_NORTH, phase);
            }

            return false;

        case DeviceType::None:
        default:
            return false;
        }
    }

    static bool queryStickButton(StickButton stickButton, ButtonPhase phase, int player)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        const PlayerBinding binding = input->getPlayerBinding(player);

        switch (binding.deviceType)
        {
        case DeviceType::Keyboard:
            return stickButton == StickButton::Left ? queryKeyboardKey(KeyCode::F, phase) : queryKeyboardKey(KeyCode::Tab, phase);

        case DeviceType::Gamepad:
            return stickButton == StickButton::Left ? queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_LEFT_STICK, phase) : queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_RIGHT_STICK, phase);

        case DeviceType::None:
        default:
            return false;
        }
    }

    static bool queryShoulderButton(ShoulderButton shoulderButton, ButtonPhase phase, int player)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        const PlayerBinding binding = input->getPlayerBinding(player);

        switch (binding.deviceType)
        {
        case DeviceType::Keyboard:
            return shoulderButton == ShoulderButton::Left ? queryKeyboardKey(KeyCode::LeftShift, phase) : queryRightMouseButton(phase);

        case DeviceType::Gamepad:
            return shoulderButton == ShoulderButton::Left ? queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, phase) : queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, phase);

        case DeviceType::None:
        default:
            return false;
        }
    }

    static bool queryTriggerButton(TriggerButton triggerButton, ButtonPhase phase, int player)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        const PlayerBinding binding = input->getPlayerBinding(player);

        switch (binding.deviceType)
        {
        case DeviceType::Keyboard:
            return triggerButton == TriggerButton::Left ? queryKeyboardKey(KeyCode::E, phase) : queryLeftMouseButton(phase);

        case DeviceType::Gamepad:
            return queryGamepadTrigger(input, triggerButton, binding.deviceIndex, phase);

        case DeviceType::None:
        default:
            return false;
        }
    }

    static bool queryPauseButton(ButtonPhase phase, int player)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        const PlayerBinding binding = input->getPlayerBinding(player);

        switch (binding.deviceType)
        {
        case DeviceType::Keyboard:
            return queryKeyboardKey(KeyCode::Escape, phase);

        case DeviceType::Gamepad:
            return queryGamepadButton(input, binding.deviceIndex, SDL_GAMEPAD_BUTTON_START, phase);

        case DeviceType::None:
        default:
            return false;
        }
    }

    bool isKeyDown(KeyCode key)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return false;
        }

        return input->isKeyDown(toKeyboardKey(key));
    }

    Vector2 getMoveAxis(int player)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return Vector2(0.0f, 0.0f);
        }

        const PlayerBinding binding = input->getPlayerBinding(player);

        switch (binding.deviceType)
        {
        case DeviceType::Keyboard:
        {
            Vector2 keyboardAxis(0.0f, 0.0f);

            if (isKeyDown(KeyCode::A))
            {
                keyboardAxis.x -= 1.0f;
            }
            if (isKeyDown(KeyCode::D))
            {
                keyboardAxis.x += 1.0f;
            }
            if (isKeyDown(KeyCode::W))
            {
                keyboardAxis.y -= 1.0f;
            }
            if (isKeyDown(KeyCode::S))
            {
                keyboardAxis.y += 1.0f;
            }

            if (keyboardAxis.LengthSquared() > 1.0f)
            {
                keyboardAxis.Normalize();
            }

            return keyboardAxis;
        }

        case DeviceType::Gamepad:
            return input->getLeftStick(binding.deviceIndex);

        case DeviceType::None:
        default:
            return Vector2(0.0f, 0.0f);
        }
    }

    Vector2 getLookAxis(int player)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return Vector2(0.0f, 0.0f);
        }

        const PlayerBinding binding = input->getPlayerBinding(player);

        switch (binding.deviceType)
        {
        case DeviceType::Keyboard:
        {
            float deltaX = 0.0f;
            float deltaY = 0.0f;
            input->getMouseDelta(deltaX, deltaY);

            return Vector2(deltaX, deltaY);
        }

        case DeviceType::Gamepad:
            return input->getRightStick(binding.deviceIndex);

        case DeviceType::None:
        default:
            return Vector2(0.0f, 0.0f);
        }
    }

    bool isLeftStickPressed(int player)
    {
        return queryStickButton(StickButton::Left, ButtonPhase::Pressed, player);
    }

    bool isRightStickPressed(int player)
    {
        return queryStickButton(StickButton::Right, ButtonPhase::Pressed, player);
    }

    bool isLeftStickJustPressed(int player)
    {
        return queryStickButton(StickButton::Left, ButtonPhase::JustPressed, player);
    }

    bool isRightStickJustPressed(int player)
    {
        return queryStickButton(StickButton::Right, ButtonPhase::JustPressed, player);
    }

    bool isLeftStickReleased(int player)
    {
        return queryStickButton(StickButton::Left, ButtonPhase::Released, player);
    }

    bool isRightStickReleased(int player)
    {
        return queryStickButton(StickButton::Right, ButtonPhase::Released, player);
    }

    bool isFaceButtonBottomPressed(int player)
    {
        return queryFaceButton(FaceButton::Bottom, ButtonPhase::Pressed, player);
    }

    bool isFaceButtonRightPressed(int player)
    {
        return queryFaceButton(FaceButton::Right, ButtonPhase::Pressed, player);
    }

    bool isFaceButtonLeftPressed(int player)
    {
        return queryFaceButton(FaceButton::Left, ButtonPhase::Pressed, player);
    }

    bool isFaceButtonTopPressed(int player)
    {
        return queryFaceButton(FaceButton::Top, ButtonPhase::Pressed, player);
    }

    bool isFaceButtonBottomJustPressed(int player)
    {
        return queryFaceButton(FaceButton::Bottom, ButtonPhase::JustPressed, player);
    }

    bool isFaceButtonRightJustPressed(int player)
    {
        return queryFaceButton(FaceButton::Right, ButtonPhase::JustPressed, player);
    }

    bool isFaceButtonLeftJustPressed(int player)
    {
        return queryFaceButton(FaceButton::Left, ButtonPhase::JustPressed, player);
    }

    bool isFaceButtonTopJustPressed(int player)
    {
        return queryFaceButton(FaceButton::Top, ButtonPhase::JustPressed, player);
    }

    bool isFaceButtonBottomReleased(int player)
    {
        return queryFaceButton(FaceButton::Bottom, ButtonPhase::Released, player);
    }

    bool isFaceButtonRightReleased(int player)
    {
        return queryFaceButton(FaceButton::Right, ButtonPhase::Released, player);
    }

    bool isFaceButtonLeftReleased(int player)
    {
        return queryFaceButton(FaceButton::Left, ButtonPhase::Released, player);
    }

    bool isFaceButtonTopReleased(int player)
    {
        return queryFaceButton(FaceButton::Top, ButtonPhase::Released, player);
    }

    bool isLeftShoulderPressed(int player)
    {
        return queryShoulderButton(ShoulderButton::Left, ButtonPhase::Pressed, player);
    }

    bool isRightShoulderPressed(int player)
    {
        return queryShoulderButton(ShoulderButton::Right, ButtonPhase::Pressed, player);
    }

    bool isLeftShoulderJustPressed(int player)
    {
        return queryShoulderButton(ShoulderButton::Left, ButtonPhase::JustPressed, player);
    }

    bool isRightShoulderJustPressed(int player)
    {
        return queryShoulderButton(ShoulderButton::Right, ButtonPhase::JustPressed, player);
    }

    bool isLeftShoulderReleased(int player)
    {
        return queryShoulderButton(ShoulderButton::Left, ButtonPhase::Released, player);
    }

    bool isRightShoulderReleased(int player)
    {
        return queryShoulderButton(ShoulderButton::Right, ButtonPhase::Released, player);
    }

    bool isLeftTriggerPressed(int player)
    {
        return queryTriggerButton(TriggerButton::Left, ButtonPhase::Pressed, player);
    }

    bool isRightTriggerPressed(int player)
    {
        return queryTriggerButton(TriggerButton::Right, ButtonPhase::Pressed, player);
    }

    bool isLeftTriggerJustPressed(int player)
    {
        return queryTriggerButton(TriggerButton::Left, ButtonPhase::JustPressed, player);
    }

    bool isRightTriggerJustPressed(int player)
    {
        return queryTriggerButton(TriggerButton::Right, ButtonPhase::JustPressed, player);
    }

    bool isLeftTriggerReleased(int player)
    {
        return queryTriggerButton(TriggerButton::Left, ButtonPhase::Released, player);
    }

    bool isRightTriggerReleased(int player)
    {
        return queryTriggerButton(TriggerButton::Right, ButtonPhase::Released, player);
    }

    bool isPausePressed(int player)
    {
        return queryPauseButton(ButtonPhase::Pressed, player);
    }

    bool isPauseJustPressed(int player)
    {
        return queryPauseButton(ButtonPhase::JustPressed, player);
    }

    bool isPauseReleased(int player)
    {
        return queryPauseButton(ButtonPhase::Released, player);
    }

    void setPlayerKeyboard(int player)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return;
        }

        input->setPlayerBinding(player, DeviceType::Keyboard, 0);
    }

    void setPlayerGamepad(int player, int gamepadIndex)
    {
        ModuleInput* input = app->getModuleInput();
        if (!input)
        {
            return;
        }

        input->setPlayerBinding(player, DeviceType::Gamepad, gamepadIndex);
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

    void setTimeScale(float timeScale)
    {
        if (!app)
        {
            return;
        }

        if (!app->getModuleTime())
        {
            return;
        }

        app->getModuleTime()->setTimeScale(timeScale);
    }

    float getTimeScale()
    {
        if (!app)
        {
            return 0.0f;
        }

        if (!app->getModuleTime())
        {
            return 0.0f;
        }

        return app->getModuleTime()->timeScale();
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

namespace CameraAPI
{
    CameraComponent* getCameraComponent(GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
    }

    const CameraComponent* getCameraComponent(const GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<CameraComponent>(ComponentType::CAMERA);
    }

    float getFov(const CameraComponent* camera)
    {
        return camera ? camera->getFov() : 0.0f;
    }

    void setFov(CameraComponent* camera, float fov)
    {
        if (!camera)
        {
            return;
        }

        camera->setFov(fov);
    }

    float getNearPlane(const CameraComponent* camera)
    {
        return camera ? camera->getNearPlane() : 0.0f;
    }

    void setNearPlane(CameraComponent* camera, float nearPlane)
    {
        if (!camera)
        {
            return;
        }

        camera->setNearPlane(nearPlane);
    }

    float getFarPlane(const CameraComponent* camera)
    {
        return camera ? camera->getFarPlane() : 0.0f;
    }

    void setFarPlane(CameraComponent* camera, float farPlane)
    {
        if (!camera)
        {
            return;
        }

        camera->setFarPlane(farPlane);
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

    bool samplePosition(const Vector3& inputPosition, Vector3& outSampledPosition, const Vector3& searchExtents, NavAgentProfile profile)
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

        unsigned short includeFlags = navigation->getIncludeFlagsForProfile(profile);

        dtQueryFilter filter;
        filter.setIncludeFlags(includeFlags);
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

        if (navigation->isPointBlockedByRuntimeBlockers(outSampledPosition))
        {
            return false;
        }

        return true;
    }

    bool moveAlongSurface(const Vector3& startPosition, const Vector3& targetPosition, Vector3& outResultPosition, const Vector3& searchExtents, NavAgentProfile profile)
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

        unsigned short includeFlags = navigation->getIncludeFlagsForProfile(profile);

        dtQueryFilter filter;
        filter.setIncludeFlags(includeFlags);
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

        if (navigation->isSegmentBlockedByRuntimeBlockers(startPosition, targetPosition))
        {
            outResultPosition = startPosition;
            return false;
        }

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

    int findStraightPath(const Vector3& startPosition, const Vector3& endPosition, Vector3* outputPoints, int maxPoints, const Vector3& searchExtents, NavAgentProfile profile)
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
        if (!navigation->findStraightPath(startPosition, endPosition, path, searchExtents, profile))
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

    bool canReachTarget(const Vector3& startPosition, const Vector3& endPosition, const Vector3& searchExtents, NavAgentProfile profile)
    {
        ModuleNavigation* navigation = app->getModuleNavigation();
        if (!navigation || !navigation->hasNavMesh())
        {
            return false;
        }

        std::vector<Vector3> path;
        if (!navigation->findStraightPath(startPosition, endPosition, path, searchExtents, profile))
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

    bool findRandomReachablePointAround(const Vector3& centerPosition, float radius, Vector3& outPoint, const Vector3& searchExtents, int maxAttempts, NavAgentProfile profile)
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

        for (int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            const float angle = ((float)std::rand() / (float)RAND_MAX) * MathAPI::TWO_PI;

            const float t = (float)std::rand() / (float)RAND_MAX;
            const float distance = std::sqrt(t) * radius;

            const Vector3 candidatePosition(centerPosition.x + std::cos(angle) * distance, centerPosition.y, centerPosition.z + std::sin(angle) * distance);

            Vector3 sampledPosition;
            if (!samplePosition(candidatePosition, sampledPosition, searchExtents, profile))
            {
                continue;
            }

            if (!canReachTarget(centerPosition, sampledPosition, searchExtents, profile))
            {
                continue;
            }

            outPoint = sampledPosition;
            return true;
        }

        return false;
    }

    bool isSegmentBlocked(const Vector3& from, const Vector3& to)
    {
        ModuleNavigation* navigation = app->getModuleNavigation();

        if (!navigation || !navigation->hasNavMesh())
        {
            return false;
        }

        return navigation->isSegmentBlockedByRuntimeBlockers(from, to);
    }

    bool canMoveSegment(const Vector3& from, const Vector3& to)
    {
        return !isSegmentBlocked(from, to);
    }

    NavRuntimeBlockerComponent* getRuntimeBlockerComponent(GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<NavRuntimeBlockerComponent>(ComponentType::NAV_RUNTIME_BLOCKER);
    }

    const NavRuntimeBlockerComponent* getRuntimeBlockerComponent(const GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<NavRuntimeBlockerComponent>(ComponentType::NAV_RUNTIME_BLOCKER);
    }

    bool isBlocked(const NavRuntimeBlockerComponent* blocker)
    {
        if (blocker->isBlocked())
        {
            return true;
        }

        return false;
    }

    void setBlocked(NavRuntimeBlockerComponent* blocker, bool blocked)
    {
        if (!blocker)
        {
            return;
        }

        blocker->setBlocked(blocked);
    }
}

namespace MathAPI
{
    float lerp(float a, float b, float t)
    {
        return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
    }
    Vector3 lerp(const Vector3& a, const Vector3& b, float t)
    {
        return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
    }
    Vector2 lerp(const Vector2& a, const Vector2& b, float t)
    {
        return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
    }

    Vector3 MathAPI::catmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t)
    {
        const float t2 = t * t;
        const float t3 = t2 * t;

        return (p1 * 2.0f + (p2 - p0) * t + (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 + (p1 * 3.0f - p0 - p2 * 3.0f + p3) * t3) * 0.5f;
    }

    float smoothStep(float edge0, float edge1, float x)
    {
        x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return x * x * (3.0f - 2.0f * x);
    }
    float pingPong(float t)
    {
        return 1.0f - std::fabsf(2.0f * t - 1.0f);
    }

    float evaluateEasing(EasingType type, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);

        switch (type)
        {
        case EasingType::EaseInQuad:
            return t * t;
        case EasingType::EaseOutQuad:
            return t * (2.0f - t);
        case EasingType::EaseInOutQuad:
        {
            float x = -2.0f * t + 2.0f;
            return t < 0.5f ? 2.0f * t * t : 1.0f - (x * x) / 2.0f;
        }
        case EasingType::EaseInCubic:
            return t * t * t;
        case EasingType::EaseOutCubic:
        {
            float x = 1.0f - t;
            return 1.0f - x * x * x;
        }
        case EasingType::EaseInOutCubic:
        {
            float x = -2.0f * t + 2.0f;
            return t < 0.5f
                ? 4.0f * t * t * t
                : 1.0f - (x * x * x) / 2.0f;
        }
        case EasingType::EaseInQuart:
            return t * t * t * t;
        case EasingType::EaseOutQuart:
        {
            float x = 1.0f - t;
            return 1.0f - x * x * x * x;
        }
        case EasingType::EaseInOutQuart:
        {
            float x = -2.0f * t + 2.0f;
            return t < 0.5f
                ? 8.0f * t * t * t * t
                : 1.0f - (x * x * x * x) / 2.0f;
        }
        case EasingType::EaseInQuint:
            return t * t * t * t * t;
        case EasingType::EaseOutQuint:
        {
            float x = 1.0f - t;
            return 1.0f - x * x * x * x * x;
        }
        case EasingType::EaseInOutQuint:
        {
            float x = -2.0f * t + 2.0f;
            return t < 0.5f
                ? 16.0f * t * t * t * t * t
                : 1.0f - (x * x * x * x * x) / 2.0f;
        }
        case EasingType::EaseInSine:
            return 1.0f - cosf(t * PI * 0.5f);
        case EasingType::EaseOutSine:
            return sinf(t * PI * 0.5f);
        case EasingType::EaseInOutSine:
            return -(cosf(PI * t) - 1.0f) / 2.0f;
        case EasingType::EaseInExpo:
            return t == 0.0f ? 0.0f : powf(2.0f, 10.0f * t - 10.0f);
        case EasingType::EaseOutExpo:
            return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
        case EasingType::EaseInOutExpo:
            return t == 0.0f
                ? 0.0f
                : t == 1.0f
                ? 1.0f
                : t < 0.5f
                ? powf(2.0f, 20.0f * t - 10.0f) / 2.0f
                : (2.0f - powf(2.0f, -20.0f * t + 10.0f)) / 2.0f;
        case EasingType::EaseInCirc:
            return 1.0f - sqrtf(1.0f - t * t);
        case EasingType::EaseOutCirc:
        {
            float x = t - 1.0f;
            return sqrtf(1.0f - x * x);
        }
        case EasingType::EaseInOutCirc:
        {
            float x1 = 2.0f * t;
            float x2 = -2.0f * t + 2.0f;

            return t < 0.5f
                ? (1.0f - sqrtf(1.0f - x1 * x1)) / 2.0f
                : (sqrtf(1.0f - x2 * x2) + 1.0f) / 2.0f;
        }
        default:
            return t;
        }
    }

    float moveTowards(float current, float target, float maxDelta)
    {
        float delta = target - current;

        if (std::fabsf(delta) <= maxDelta)
        {
            return target;
        }

        return current + (delta > 0.0f ? maxDelta : -maxDelta);
    }
}

namespace Transform2DAPI
{
    Vector2 getPosition(const Transform2D* transform)
    {
        if (!transform)
        {
            return Vector2(0.0f, 0.0f);
        }
		return transform->getPosition();
    }
    void setPosition(Transform2D* transform, const Vector2& newPosition)
    {
        if (!transform)
        {
            return;
        }
        transform->setPosition(newPosition);
    }
    Vector2 getScale(const Transform2D* transform)
    {
        if (!transform)
        {
            return Vector2(1.0f, 1.0f);
        }
        return transform->getScale();
	}
    void setScale(Transform2D* transform, const Vector2& newScale)
    {
        if (!transform)
        {
            return;
        }
        transform->setScale(newScale);
    }
    float getAlpha(const Transform2D* transform)
    {
        if (!transform)
        {
            return 1.0f;
        }
        return transform->getAlpha();
	}
    void setAlpha(Transform2D* transform, float newAlpha)
    {
        if (!transform)
        {
            return;
        }
        transform->setAlpha(newAlpha);
	}
    Vector2 getPivot(const Transform2D* transform)
    {
        if (!transform)
        {
            return Vector2(0.5f, 0.5f);
        }
		return transform->getPivot();
    }
    void setPivot(Transform2D* transform, const Vector2& newPivot)
    {
        if (!transform)
        {
            return;
        }
        transform->setPivot(newPivot);
    }
    Vector2 getAnchorMin(const Transform2D* transform)
    {
        if (!transform)
        {
            return Vector2(0.0f, 0.0f);
        }
		return transform->getAnchorMin();
    }
    void setAnchorMin(Transform2D* transform, const Vector2& newAnchorMin)
    {
        if (!transform)
        {
            return;
        }
        transform->setAnchorMin(newAnchorMin);
	}
    Vector2 getAnchorMax(const Transform2D* transform)
    {
        if (!transform)
        {
            return Vector2(1.0f, 1.0f);
        }
        return transform->getAnchorMax();
    }
    void setAnchorMax(Transform2D* transform, const Vector2& newAnchorMax)
    {
        if (!transform)
        {
            return;
        }
        transform->setAnchorMax(newAnchorMax);
	}
    Vector2 getBaseSize(const Transform2D* transform)
    {
        if (!transform)
        {
            return Vector2(0.0f, 0.0f);
        }
		return transform->getBaseSize();
    }
    void setBaseSize(Transform2D* transform, const Vector2& newBaseSize)
    {
        if (!transform)
        {
            return;
        }
        transform->setBaseSize(newBaseSize);
    }
}

namespace SliderAPI
{
    float getFillAmount(const UISlider* slider)
    {
        if (!slider)
        {
            return 0.0f;
        }

        return slider->getFillAmount().y;
    }

    void setFillAmount(UISlider* slider, const float amount)
    {
        if (!slider)
        {
            return;
        }

        Vector2 v = slider->getFillAmount();
        v.y = amount;
        slider->setFillAmount(v);
    }

    Vector2 getFillAmountVec(const UISlider* slider)
    {
        if (!slider)
        {
            return Vector2(0.0f, 0.0f);
        }
        return slider->getFillAmount();
    }

    void setFillAmountVec(UISlider* slider, const Vector2& amount)
    {
        if (!slider) return;
        slider->setFillAmount(amount);
    }

    FillMethod getFillMethod(const UISlider* slider)
    {
        if (!slider)
        {
            return FillMethod::Horizontal;
        }
        return slider->getFillMethod();
	}

    void setFillMethod(UISlider* slider, FillMethod method)
    {
        if (!slider)
        {
            return;
        }
        slider->setFillMethod(method);
	}

    FillOrigin getFillOrigin(const UISlider* slider)
    {
        if (!slider)
        {
            return FillOrigin::HorizontalLeft;
        }
        return slider->getFillOrigin();
    }

    void setFillOrigin(UISlider* slider, FillOrigin origin)
    {
        if (!slider)
        {
            return;
        }
        slider->setFillOrigin(origin);
	}
}

namespace UISheetAPI
{
    void play(UISheet* sheet)
    {
        if (!sheet)
        {
            return;
        }
        sheet->play();
    }

    void stop(UISheet* sheet)
    {
        if (!sheet)
        {
            return;
        }
        sheet->stop();
    }
    
    void playReverse(UISheet* sheet)
    {
        if (!sheet)
        {
            return;
        }
        sheet->playReverse();
    }
    
    bool getLoop(UISheet* sheet)
    {
        if (!sheet)
        {
            return false;
        }
        return sheet->getLoop();
    }
    
    void setLoop(UISheet* sheet, bool isLoop)
    {
        if (!sheet)
        {
            return;
        }
        sheet->setLoop(isLoop);
    }

    bool isPlaying(UISheet* sheet)
    {
        if (!sheet)
        {
            return false;
        }
        return sheet->isPlaying();
    }

    Vector2 getOffset(UISheet* sheet)
    {
        if (!sheet)
        {
            return Vector2(0.0f, 0.0f);
        }
        return sheet->getOffset();
    }

    void setOffset(UISheet* sheet, const Vector2& offset)
    {
        if (!sheet)
        {
            return;
        }
        sheet->setOffset(offset);
    }

    void reset(UISheet* sheet)
    {
        if (!sheet)
        {
            return;
        }
		sheet->reset();
	}
}

namespace DebugDrawAPI
{
    void drawPoint(const Vector3& pos, const Vector3& color, float size, int durationMillis, bool depthEnabled)
    {
        dd::point(ddConvert(pos), ddConvert(color), size, durationMillis, depthEnabled);
    }

    void drawLine(const Vector3& from, const Vector3& to, const Vector3& color, int durationMillis, bool depthEnabled)
    {
        dd::line(ddConvert(from), ddConvert(to), ddConvert(color), durationMillis, depthEnabled);
    }

    void drawScreenText(const char* str, const Vector3& pos, const Vector3& color, float scaling, int durationMillis)
    {
        dd::screenText(str, ddConvert(pos), ddConvert(color), scaling, durationMillis);
    }

    void drawProjectedText(const char* str, const Vector3& pos, const Vector3& color, const Matrix& vpMatrix, int sx, int sy, int sw, int sh, float scaling, int durationMillis)
    {
        dd::projectedText(str, ddConvert(pos), ddConvert(color), ddConvert(vpMatrix), sx, sy, sw, sh, scaling, durationMillis);
    }

    void drawAxisTriad(const Matrix& transform, float size, float length, int durationMillis, bool depthEnabled)
    {
        dd::axisTriad(ddConvert(transform), size, length, durationMillis, depthEnabled);
    }

    void drawArrow(const Vector3& from, const Vector3& to, const Vector3& color, float size, int durationMillis, bool depthEnabled)
    {
        dd::arrow(ddConvert(from), ddConvert(to), ddConvert(color), size, durationMillis, depthEnabled);
    }

    void drawCross(const Vector3& center, float length, int durationMillis, bool depthEnabled)
    {
        dd::cross(ddConvert(center), length, durationMillis, depthEnabled);
    }

    void drawCircle(const Vector3& center, const Vector3& planeNormal, const Vector3& color, float radius, float numSteps, int durationMillis, bool depthEnabled)
    {
        dd::circle(ddConvert(center), ddConvert(planeNormal), ddConvert(color), radius, numSteps, durationMillis, depthEnabled);
    }

    void drawPlane(const Vector3& center, const Vector3& planeNormal, const Vector3& planeColor, const Vector3& normalVecColor, float planeScale, float normalVecScale, int durationMillis, bool depthEnabled)
    {
        dd::plane(ddConvert(center), ddConvert(planeNormal), ddConvert(planeColor), ddConvert(normalVecColor), planeScale, normalVecScale, durationMillis, depthEnabled);
    }

    void drawSphere(const Vector3& center, const Vector3& color, float radius, int durationMillis, bool depthEnabled)
    {
        dd::sphere(ddConvert(center), ddConvert(color), radius, durationMillis, depthEnabled);
    }

    void drawCone(const Vector3& apex, const Vector3& dir, const Vector3& color, float baseRadius, float apexRadius, int durationMillis, bool depthEnabled)
    {
        dd::cone(ddConvert(apex), ddConvert(dir), ddConvert(color), baseRadius, apexRadius, durationMillis, depthEnabled);
    }

    void drawBox(const Vector3& center, const Vector3& color, float width, float height, float depth, int durationMillis, bool depthEnabled)
    {
        dd::box(ddConvert(center), ddConvert(color), width, height, depth, durationMillis, depthEnabled);
    }

    void drawAABB(const Vector3& mins, const Vector3& maxs, const Vector3& color, int durationMillis, bool depthEnabled)
    {
        dd::aabb(ddConvert(mins), ddConvert(maxs), ddConvert(color), durationMillis, depthEnabled);
    }

    void drawFrustum(const Matrix& invClipMatrix, const Vector3& color, int durationMillis, bool depthEnabled)
    {
        dd::frustum(ddConvert(invClipMatrix), ddConvert(color), durationMillis, depthEnabled);
    }

    void drawVertexNormal(const Vector3& origin, const Vector3& normal, float length, int durationMillis, bool depthEnabled)
    {
        dd::vertexNormal(ddConvert(origin), ddConvert(normal), length, durationMillis, depthEnabled);
    }

    void drawTangentBasis(const Vector3& origin, const Vector3& normal, const Vector3& tangent, const Vector3& bitangent, float lengths, int durationMillis, bool depthEnabled)
    {
        dd::tangentBasis(ddConvert(origin), ddConvert(normal), ddConvert(tangent), ddConvert(bitangent), lengths, durationMillis, depthEnabled);
    }

    void drawXZSquareGrid(float mins, float maxs, float y, float step, const Vector3& color, int durationMillis, bool depthEnabled)
    {
        dd::xzSquareGrid(mins, maxs, y, step, ddConvert(color), durationMillis, depthEnabled);
    }
}

namespace HapticAPI
{
    uint32_t playEffect(const char* effectId, int player)
    {
        if (!effectId || !app)
        {
            return 0;
        }

        ModuleHaptics* haptics = app->getModuleHaptics();
        if (!haptics)
        {
            return 0;
        }

        return haptics->playEffect(effectId, player);
    }

    uint32_t playAtScale(const char* effectId, float scale, int player)
    {
        if (!effectId || !app)
        {
            return 0;
        }

        ModuleHaptics* haptics = app->getModuleHaptics();
        if (!haptics)
        {
            return 0;
        }

        return haptics->playAtScale(effectId, scale, player);
    }

    void stopEffect(uint32_t handle, int player)
    {
        if (!app || handle == 0)
        {
            return;
        }

        ModuleHaptics* haptics = app->getModuleHaptics();
        if (!haptics)
        {
            return;
        }

        haptics->cancelEffect(handle, player);
    }

    void stopAll(int player)
    {
        if (!app)
        {
            return;
        }

        ModuleHaptics* haptics = app->getModuleHaptics();
        if (!haptics)
        {
            return;
        }

        haptics->cancelAll(player);
    }

    bool isPlaying(int player)
    {
        if (!app)
        {
            return false;
        }

        ModuleHaptics* haptics = app->getModuleHaptics();
        if (!haptics)
        {
            return false;
        }

        return haptics->isPlaying(player);
    }

    uint32_t submitImpact(float intensity, float duration, int player)
    {
        if (!app)
        {
            return 0;
        }

        ModuleHaptics* haptics = app->getModuleHaptics();
        if (!haptics)
        {
            return 0;
        }

        return haptics->submitAnonymous(HapticEffectDefinition::makeImpact(intensity, duration), 1.0f, player);
    }

    uint32_t HapticAPI::submitEffect(const HapticEffectDefinition& def, int player)
    {
        return app->getModuleHaptics()->submitEffect(def, player);
    }

    uint32_t submitRumble(float left, float right, float duration, int player)
    {
        if (!app)
        {
            return 0;
        }

        ModuleHaptics* haptics = app->getModuleHaptics();
        if (!haptics)
        {
            return 0;
        }

        return haptics->submitAnonymous(HapticEffectDefinition::makeContinuous(left, right, duration), 1.0f, player);
    }

    uint32_t submitExplosion(float intensity, float duration, int player)
    {
        if (!app)
        {
            return 0;
        }

        ModuleHaptics* haptics = app->getModuleHaptics();
        if (!haptics)
        {
            return 0;
        }

        return haptics->submitAnonymous(HapticEffectDefinition::makeExplosion(intensity, duration), 1.0f, player);
    }

    void cancelEffect(uint32_t handle, int player)
    {
        stopEffect(handle, player);
    }

    void cancelAll(int player)
    {
        stopAll(player);
    }

    void registerEffect(const HapticEffectDefinition& def)
    {
        HapticEffectLibrary::get().registerEffect(def);
    }

    bool saveToJSON(const char* path)
    {
        return HapticEffectLibrary::get().saveToJSON(path);
    }

    const HapticEffectDefinition* findEffect(const char* id)
    {
        return HapticEffectLibrary::get().findEffect(id);
    }
}

ENGINE_API ParticleSystemComponent* ParticleSystemAPI::getParticleSystemComponent(GameObject* gameObject)
{
    if (!gameObject)
    {
        return nullptr;
    }

    return gameObject->GetComponentAs<ParticleSystemComponent>(ComponentType::PARTICLE_SYSTEM);
}

ENGINE_API const ParticleSystemComponent* ParticleSystemAPI::getParticleSystemComponent(const GameObject* gameObject)
{
    if (!gameObject)
    {
        return nullptr;
    }

    return gameObject->GetComponentAs<ParticleSystemComponent>(ComponentType::PARTICLE_SYSTEM);
}

ENGINE_API void ParticleSystemAPI::play(ParticleSystemComponent* particleSystem)
{
    if (!particleSystem)
    {
        return;
    }

    particleSystem->setLocalTimeScale(1.f);
}

ENGINE_API void ParticleSystemAPI::pause(ParticleSystemComponent* particleSystem)
{
    if (!particleSystem)
    {
        return;
    }

    particleSystem->setLocalTimeScale(0.f);
}

ENGINE_API void ParticleSystemAPI::stop(ParticleSystemComponent* particleSystem)
{
    if (!particleSystem)
    {
        return;
    }

    particleSystem->resetParticles();
    particleSystem->setLocalTimeScale(0.f);
}

ENGINE_API bool ParticleSystemAPI::isPlaying(ParticleSystemComponent* particleSystem)
{
    return particleSystem ? particleSystem->getLocalTimeScale() > 0.f : false;
}

ENGINE_API void ParticleSystemAPI::reset(ParticleSystemComponent* particleSystem)
{
    if (!particleSystem)
    {
        return;
    }

    particleSystem->resetParticles();
}

namespace TrailAPI
{
    TrailComponent* getTrailComponent(GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<TrailComponent>(ComponentType::TRAIL);
    }

    const TrailComponent* getTrailComponent(const GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<TrailComponent>(ComponentType::TRAIL);
    }

    bool isTrailGenerating(TrailComponent* trailComponent)
    {
        return trailComponent->isGenerating();
    }

    void generateTrail(TrailComponent* trailComponent, bool value)
    {
        return trailComponent->generate(value);
    }
}

namespace AudioAPI
{
    ComponentSoundSource* getSoundSourceComponent(GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<ComponentSoundSource>(ComponentType::SOUND_SOURCE);
    }

    const ComponentSoundSource* getSoundSourceComponent(const GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<ComponentSoundSource>(ComponentType::SOUND_SOURCE);
    }

    uint32_t postEvent(ComponentSoundSource* component, const char* bankName, const char* eventName)
    {
        if (component)
        {
            return component->postEvent(bankName, eventName);
        }
        return NULL;
    }

    void stopEvent(ComponentSoundSource* component, uint32_t playingID)
    {
        if (component)
        {
            component->stopEvent(playingID);
        }
    }

    void pauseEvent(ComponentSoundSource* component, uint32_t playingID)
    {
        if (component)
        {
            component->pauseEvent(playingID);
        }
    }

    void resumeEvent(ComponentSoundSource* component, uint32_t playingID)
    {
        if (component)
        {
            component->resumeEvent(playingID);
        }
    }

    void setState(const char* stateGroup, const char* stateValue)
    {
		app->getModuleMusic()->setState(stateGroup, stateValue);
    }

    void setSwitch(const char* switchGroup, const char* switchValue, ComponentSoundSource* component)
    {
        component->setSwitch(switchGroup, switchValue);
    }

    void setRTPC(const char* rtpcName, float value)
    {
		app->getModuleMusic()->setRTPC(rtpcName, value);
    }

    bool isMusicStarted()
    {
		return app->getModuleMusic()->isMusicStarted();
    }

    void setMusicStarted(bool started)
    {
		app->getModuleMusic()->setMusicStarted(started);
    }
}

namespace Shaders
{
    PlayerRenderBufferComponent* Shaders::getPlayerRenderBufferComponent(GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<PlayerRenderBufferComponent>(ComponentType::PLAYER_RENDER_BUFFER);
    }

    const PlayerRenderBufferComponent* Shaders::getPlayerRenderBufferComponent(const GameObject* gameObject)
    {
        if (!gameObject)
        {
            return nullptr;
        }

        return gameObject->GetComponentAs<PlayerRenderBufferComponent>(ComponentType::PLAYER_RENDER_BUFFER);
    }

    float Shaders::getDamageHighlightIntensity(PlayerRenderBufferComponent* component)
    {
        return component->getDamageHighlightIntensity();
    }

    void setDamageHighlightIntensity(PlayerRenderBufferComponent* component, float value)
    {
        component->setDamageHighlightIntensity(value);
    }

    Vector3 Shaders::getDamageHighlightCenterColor(PlayerRenderBufferComponent* component)
    {
        return component->getDamageHighlightCenterColor();
    }
    
    void Shaders::setDamageHighlightCenterColor(PlayerRenderBufferComponent* component, Vector3 value)
    {
        return component->setDamageHighlightCenterColor(value);
    }

    Vector3 Shaders::getDamageHighlightRimColor(PlayerRenderBufferComponent* component)
    {
        return component->getDamageHighlightRimColor();
    }

    void Shaders::setDamageHighlightRimColor(PlayerRenderBufferComponent* component, Vector3 value)
    {
        return component->setDamageHighlightRimColor(value);
    }

    float Shaders::getDamageHighlightRimIntensity(PlayerRenderBufferComponent* component)
    {
        return component->getDamageHighlightRimIntensity();
    }

    void setDamageHighlightRimIntensity(PlayerRenderBufferComponent* component, float value)
    {
        component->setDamageHighlightRimIntensity(value);
    }
}
