#include "pch.h"
#include "CameraTransitionStep.h"
#include "CameraTransitionStepAction.h"

static const char* cameraStepMoveModeNames[] =
{
    "Linear",
    "Smooth",
    "Catmull Rom"
};

constexpr int cameraStepMoveModeCount = 3;

IMPLEMENT_SCRIPT_FIELDS(CameraTransitionStep,
    SERIALIZED_ENUM_INT(m_moveMode, "Move Mode", cameraStepMoveModeNames, cameraStepMoveModeCount),

    SERIALIZED_FLOAT(m_moveDuration, "Move Duration", 0.0f, 20.0f, 0.05f),
    SERIALIZED_FLOAT(m_holdDuration, "Hold Duration", 0.0f, 20.0f, 0.05f),

    SERIALIZED_BOOL(m_useFovTransition, "Use FOV Transition"),
    SERIALIZED_FLOAT(m_targetFov, "Target FOV", 5.0f, 120.0f, 0.1f)
)

CameraTransitionStep::CameraTransitionStep(GameObject* owner)
    : Script(owner)
{
}

void CameraTransitionStep::Start()
{
    findStepActions();
}

void CameraTransitionStep::executeStepStartedActions(CameraTransitionController* controller)
{
    for (CameraTransitionStepAction* action : m_stepActions)
    {
        if (action == nullptr)
        {
            continue;
        }

        action->onStepStarted(controller, this);
    }
}

void CameraTransitionStep::executeStepReachedActions(CameraTransitionController* controller)
{
    for (CameraTransitionStepAction* action : m_stepActions)
    {
        if (action == nullptr)
        {
            continue;
        }

        action->onStepReached(controller, this);
    }
}

void CameraTransitionStep::executeStepFinishedActions(CameraTransitionController* controller)
{
    for (CameraTransitionStepAction* action : m_stepActions)
    {
        if (action == nullptr)
        {
            continue;
        }

        action->onStepFinished(controller, this);
    }
}

void CameraTransitionStep::findStepActions()
{
    m_stepActions.clear();

    GameObject* owner = getOwner();

    const int scriptCount = GameObjectAPI::getScriptCount(owner);

    for (int i = 0; i < scriptCount; ++i)
    {
        Script* script = GameObjectAPI::getScriptByIndex(owner, i);
        if (script == nullptr)
        {
            continue;
        }

        CameraTransitionStepAction* action = dynamic_cast<CameraTransitionStepAction*>(script);
        if (action == nullptr)
        {
            continue;
        }

        m_stepActions.push_back(action);
    }
}

IMPLEMENT_SCRIPT(CameraTransitionStep)