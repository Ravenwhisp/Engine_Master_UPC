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
#include "ScriptComponent.h"
#include "Script.h"

#include "CameraComponent.h"

#include "DeviceType.h"
#include "PlayerBinding.h"

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

    Transform* TransformAPI::getParent(Transform* transform)
    {
        return transform->getRoot();
    }

    const Transform* TransformAPI::getParent(const Transform* transform)
    {
        return transform->getRoot();
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
                return queryKeyboardKey(KeyCode::E, phase);

            case FaceButton::Left:
                return queryKeyboardKey(KeyCode::Q, phase);

            case FaceButton::Top:
                return queryKeyboardKey(KeyCode::R, phase);
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
            return stickButton == StickButton::Left ? queryKeyboardKey(KeyCode::Tab, phase) : queryKeyboardKey(KeyCode::F, phase);

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
            return shoulderButton == ShoulderButton::Left ? queryKeyboardKey(KeyCode::Num1, phase) : queryKeyboardKey(KeyCode::Num2, phase);

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
            return triggerButton == TriggerButton::Left ? queryKeyboardKey(KeyCode::Num3, phase) : queryKeyboardKey(KeyCode::Num4, phase);

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