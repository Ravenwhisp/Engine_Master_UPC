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

    bool isTransitioning() const { return m_isTransitioning; }

private:
    enum class TransitionState
    {
        None,
        MovingToTarget,
        Holding,
        Returning
    };

private:
    void startMovingToTarget(CameraTransitionEvent* event);
    void updateMovingToTarget(float dt);
    void updateHolding(float dt);
    void updateReturning(float dt);
    void finishTransition();

    void buildPathFromCurrentEvent();
    Vector3 evaluateCatmullRomPath(float normalizedTime) const;
    Vector3 evaluateRotationPath(float normalizedTime) const;

    Vector3 catmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) const;

    void findPlayerControllers();
    void setPlayersGameplayInputLocked(bool locked);
    void setPlayersInvulnerable(bool invulnerable);

    void findHUDFader();

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

    std::vector<Vector3> m_pathPositions;
    std::vector<Vector3> m_pathRotations;

    float m_originalFov = 90.0f;
    float m_returnStartFov = 90.0f;

    float m_hudFadeOutDuration = 0.35f;
    float m_hudFadeInDuration = 0.35f;

};