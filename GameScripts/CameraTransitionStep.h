#pragma once

#include "ScriptAPI.h"

class CameraTransitionController;
class CameraTransitionStepAction;

enum class CameraStepMoveMode
{
    Linear = 0,
    Smooth,
    CatmullRom
};

class CameraTransitionStep : public Script
{
    DECLARE_SCRIPT(CameraTransitionStep)

public:
    explicit CameraTransitionStep(GameObject* owner);

    void Start() override;

    FieldList getExposedFields() const override;

    CameraStepMoveMode getMoveMode() const { return static_cast<CameraStepMoveMode>(m_moveMode); }

    float getMoveDuration() const { return m_moveDuration; }
    float getHoldDuration() const { return m_holdDuration; }

    bool usesFovTransition() const { return m_useFovTransition; }
    float getTargetFov() const { return m_targetFov; }

    void executeStepStartedActions(CameraTransitionController* controller);
    void executeStepReachedActions(CameraTransitionController* controller);
    void executeStepFinishedActions(CameraTransitionController* controller);

private:
    void findStepActions();

public:
    int m_moveMode = static_cast<int>(CameraStepMoveMode::Smooth);

    float m_moveDuration = 1.0f;
    float m_holdDuration = 0.0f;

    bool m_useFovTransition = false;
    float m_targetFov = 90.0f;

private:
    std::vector<CameraTransitionStepAction*> m_stepActions;
};