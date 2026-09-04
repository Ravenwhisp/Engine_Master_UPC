#include "pch.h"
#include "CameraTransitionStepAction.h"

static const char* cameraStepActionTriggerNames[] =
{
    "Step Started",
    "Step Reached",
    "Step Finished"
};

constexpr int cameraStepActionTriggerCount = 3;

IMPLEMENT_SCRIPT_FIELDS(CameraTransitionStepAction,
    SERIALIZED_ENUM_INT(m_triggerMoment, "Trigger Moment", cameraStepActionTriggerNames, cameraStepActionTriggerCount),
    SERIALIZED_FLOAT(m_startDelay, "Start Delay", 0.0f, 20.0f, 0.05f)
)

CameraTransitionStepAction::CameraTransitionStepAction(GameObject* owner)
    : Script(owner)
{
}

void CameraTransitionStepAction::Update()
{
    if (!m_waitingForDelay)
    {
        return;
    }

    m_delayTimer += Time::getDeltaTime();

    if (m_delayTimer < m_startDelay)
    {
        return;
    }

    executeDelayedAction();
}

void CameraTransitionStepAction::onStepStarted(CameraTransitionController* controller, CameraTransitionStep* step)
{
    tryTriggerAction(CameraTransitionStepActionTrigger::StepStarted, controller, step);
}

void CameraTransitionStepAction::onStepReached(CameraTransitionController* controller, CameraTransitionStep* step)
{
    tryTriggerAction(CameraTransitionStepActionTrigger::StepReached, controller, step);
}

void CameraTransitionStepAction::onStepFinished(CameraTransitionController* controller, CameraTransitionStep* step)
{
    tryTriggerAction(CameraTransitionStepActionTrigger::StepFinished, controller, step);
}

void CameraTransitionStepAction::tryTriggerAction(CameraTransitionStepActionTrigger trigger, CameraTransitionController* controller, CameraTransitionStep* step)
{
    if (!shouldRunOnTrigger(trigger))
    {
        return;
    }

    if (m_startDelay <= 0.0001f)
    {
        executeAction(controller, step);
        return;
    }

    startDelay(controller, step);
}

bool CameraTransitionStepAction::shouldRunOnTrigger(CameraTransitionStepActionTrigger trigger) const
{
    return static_cast<CameraTransitionStepActionTrigger>(m_triggerMoment) == trigger;
}

void CameraTransitionStepAction::startDelay(CameraTransitionController* controller, CameraTransitionStep* step)
{
    m_waitingForDelay = true;
    m_delayTimer = 0.0f;

    m_pendingController = controller;
    m_pendingStep = step;
}

void CameraTransitionStepAction::executeDelayedAction()
{
    m_waitingForDelay = false;
    m_delayTimer = 0.0f;

    CameraTransitionController* controller = m_pendingController;
    CameraTransitionStep* step = m_pendingStep;

    m_pendingController = nullptr;
    m_pendingStep = nullptr;

    executeAction(controller, step);
}