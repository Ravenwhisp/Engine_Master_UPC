#pragma once

#include "ScriptAPI.h"

class CameraFollow;
class CameraTransitionEvent;
class PlayerController;
class Damageable;
class CameraComponent;
class HUDFader;

class CameraTransitionController : public Script
{
    DECLARE_SCRIPT(CameraTransitionController)

public:
    explicit CameraTransitionController(GameObject* owner);

    void Start() override;
    void Update() override;

    void startTransition(CameraTransitionEvent* event);
    void releaseTransition(CameraTransitionEvent* event);

    bool isTransitioning() const { return m_isTransitioning; }

private:
    enum class TransitionState
    {
        None,
        MovingStep,
        HoldingStep,
        WaitingForRelease,
        Returning
    };

private:
    void startTransitionSequence(CameraTransitionEvent* event, bool preserveOriginalFov);
    void startStep(int stepIndex);
    void startReturning();

    void updateMovingStep(float dt);
    void updateHoldingStep(float dt);
    void updateReturning(float dt);

    bool hasValidStepSequence() const;
    void finishCurrentStepMovement();
    void finishCurrentStepHold();

    Vector3 evaluateStepPosition(float alpha) const;
    Vector3 evaluateStepRotation(float alpha) const;
    Vector3 evaluateCatmullRomStepPosition(float alpha) const;

    void finishTransition();

    void findPlayerControllers();
    void setPlayersGameplayInputLocked(bool locked);
    void setPlayersInvulnerable(bool invulnerable);

    void findHUDFader();

    bool getCameraFollowReturnTarget(Vector3& outPosition, Vector3& outRotation);

private:
    CameraFollow* m_cameraFollow = nullptr;
    CameraComponent* m_camera = nullptr;
    CameraTransitionEvent* m_currentEvent = nullptr;
    HUDFader* m_hudFader = nullptr;
    std::vector<PlayerController*> m_playerControllers;
    std::vector<Damageable*> m_playerDamageables;

    TransitionState m_state = TransitionState::None;

    bool m_isTransitioning = false;

    float m_timer = 0.0f;

    Vector3 m_transitionStartPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_transitionStartRotation = Vector3(0.0f, 0.0f, 0.0f);

    Vector3 m_returnStartPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_returnStartRotation = Vector3(0.0f, 0.0f, 0.0f);

    float m_originalFov = 90.0f;
    float m_returnStartFov = 90.0f;

    float m_hudFadeOutDuration = 0.35f;
    float m_hudFadeInDuration = 0.35f;

    int m_currentStepIndex = -1;

    Vector3 m_stepStartPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_stepStartRotation = Vector3(0.0f, 0.0f, 0.0f);

    Vector3 m_stepTargetPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_stepTargetRotation = Vector3(0.0f, 0.0f, 0.0f);

    float m_stepMoveDuration = 0.0f;
    float m_stepHoldDuration = 0.0f;

    float m_stepStartFov = 90.0f;
    float m_stepTargetFov = 90.0f;
    bool m_stepUsesFovTransition = false;

};