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
    FieldList getExposedFields() const override;

    void drawGizmo() override;

protected:
	void startAbility() override;
	bool canStartSpecificAbility() const override;

    virtual bool canDash() const;
    virtual void onDashStarted();
    virtual void onDashUpdate(float dt) {}
    virtual void onDashEnded() {}
    virtual bool validateDashTarget() { return true; }

private:
    void startDash();
    void updateDash(float dt);
    void stopDash();
    void calculateDashMovement(float dt);

protected:
    PlayerController* m_playerController = nullptr;
    PlayerMovement* m_playerMovement = nullptr;

    float m_dashDuration = 0.15f;
    float m_dashDistance = 3.0f;

    float m_dashTimer = 0.0f;
    bool m_isDashing = false;

    Vector3 m_dashDirection = Vector3::Zero;

    Vector3 m_dashTargetPosition = Vector3::Zero;
    Vector3 m_dashStartPosition = Vector3::Zero;
    bool m_hasDashTarget = false;
};