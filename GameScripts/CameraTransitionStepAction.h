#pragma once

#include "ScriptAPI.h"

class CameraTransitionController;
class CameraTransitionStep;

enum class CameraTransitionStepActionTrigger
{
    StepStarted = 0,
    StepReached,
    StepFinished
};

class CameraTransitionStepAction : public Script
{
public:
    explicit CameraTransitionStepAction(GameObject* owner);

    void Update() override;


    FieldList getExposedFields() const override;

    void onStepStarted(CameraTransitionController* controller, CameraTransitionStep* step);
    void onStepReached(CameraTransitionController* controller, CameraTransitionStep* step);
    void onStepFinished(CameraTransitionController* controller, CameraTransitionStep* step);

protected:
    virtual void executeAction(CameraTransitionController* controller, CameraTransitionStep* step) {}

private:
    void tryTriggerAction(CameraTransitionStepActionTrigger trigger, CameraTransitionController* controller, CameraTransitionStep* step);

    bool shouldRunOnTrigger(CameraTransitionStepActionTrigger trigger) const;
    void startDelay(CameraTransitionController* controller, CameraTransitionStep* step);
    void executeDelayedAction();

public:
    int m_triggerMoment = static_cast<int>(CameraTransitionStepActionTrigger::StepReached);
    float m_startDelay = 0.0f;

private:
    bool m_waitingForDelay = false;
    float m_delayTimer = 0.0f;

    CameraTransitionController* m_pendingController = nullptr;
    CameraTransitionStep* m_pendingStep = nullptr;
};
