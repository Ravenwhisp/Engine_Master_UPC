#pragma once

#include "AbilityBase.h"

class PlayerController;
class PlayerMovement;

class AbilityDash : public AbilityBase
{
public:
    explicit AbilityDash(GameObject* owner);

    void Start() override;
    void Update() override;
    void drawGizmo() override;

protected:
    virtual bool canDash() const;
    virtual void onDashStarted();
    virtual void onDashUpdate(float dt) {}
    virtual void onDashEnded() {}

private:
    void tryStartDash();
    void updateDash(float dt);
    void stopDash();
    void calculateDashMovement(float dt);

    PlayerController* findControllerScript(GameObject* owner) const;
    PlayerMovement* findMovementScript(GameObject* owner) const;

protected:
    PlayerController* m_playerController = nullptr;
    PlayerMovement* m_playerMovement = nullptr;

    float m_dashDuration = 0.15f;
    float m_dashDistance = 3.0f;

    float m_dashTimer = 0.0f;
    bool m_isDashing = false;

    Vector3 m_dashDirection = Vector3::Zero;
};