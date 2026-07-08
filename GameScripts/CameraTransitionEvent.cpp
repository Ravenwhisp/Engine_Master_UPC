#include "pch.h"
#include "CameraTransitionEvent.h"

#include "GameplayEventTrigger.h"
#include "CameraTransitionController.h"
#include "CameraTransitionStep.h"

static const char* cameraTransitionModeNames[] =
{
    "Timed Cinematic",
    "Hold While Triggered"
};

constexpr int cameraTransitionModeCount = 2;

IMPLEMENT_SCRIPT_FIELDS(CameraTransitionEvent,
    SERIALIZED_ENUM_INT(m_transitionMode, "Transition Mode", cameraTransitionModeNames, cameraTransitionModeCount),
    SERIALIZED_BOOL(m_lockGameplayInput, "Lock Gameplay Input"),
    SERIALIZED_BOOL(m_makePlayersInvulnerable, "Make Players Invulnerable"),
    SERIALIZED_BOOL(m_fadeHud, "Fade HUD"),
    SERIALIZED_FLOAT(m_returnDuration, "Return Duration", 0.0f, 20.0f, 0.05f)
)

CameraTransitionEvent::CameraTransitionEvent(GameObject* owner)
    : GameplayEventAction(owner)
{
}

void CameraTransitionEvent::Start()
{
    findTargetPoints();

    if (m_targetPoints.empty())
    {
        Debug::warn("CameraTransitionEvent on '%s' could not find any points under 'CameraPoints'.", GameObjectAPI::getName(getOwner()));
    }
}

void CameraTransitionEvent::executeEvent(GameplayEventTrigger* trigger)
{
    if (m_targetPoints.empty())
    {
        return;
    }

    CameraTransitionController* cameraTransitionController = findCameraTransitionController();
    if (cameraTransitionController == nullptr)
    {
        Debug::warn("CameraTransitionEvent on '%s' could not find CameraTransitionController on the default camera.",GameObjectAPI::getName(getOwner()));
        return;
    }

    cameraTransitionController->startTransition(this);
}

void CameraTransitionEvent::stopEvent(GameplayEventTrigger* trigger)
{
    if (!isHoldWhileTriggeredMode())
    {
        return;
    }

    CameraTransitionController* cameraTransitionController = findCameraTransitionController();
    if (cameraTransitionController == nullptr)
    {
        Debug::warn("CameraTransitionEvent on '%s' could not find CameraTransitionController on the default camera.", GameObjectAPI::getName(getOwner()));
        return;
    }

    cameraTransitionController->releaseTransition(this);
}

Transform* CameraTransitionEvent::getTargetPoint(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_targetPoints.size()))
    {
        return nullptr;
    }

    return m_targetPoints[index];
}

CameraTransitionStep* CameraTransitionEvent::getTransitionStep(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_transitionSteps.size()))
    {
        return nullptr;
    }

    return m_transitionSteps[index];
}

void CameraTransitionEvent::findTargetPoints()
{
    m_targetPoints.clear();
    m_transitionSteps.clear();

    Transform* cameraPointsRoot = findCameraPointsRoot();
    if (cameraPointsRoot == nullptr)
    {
        return;
    }

    for (int i = 1; i <= 32; ++i)
    {
        char pointName[32];
        sprintf_s(pointName, "Point%d", i);

        Transform* point = TransformAPI::findChildByName(cameraPointsRoot, pointName);
        if (point == nullptr)
        {
            break;
        }

        m_targetPoints.push_back(point);

        GameObject* pointObject = ComponentAPI::getOwner(point);
        CameraTransitionStep* transitionStep = GameObjectAPI::findScript<CameraTransitionStep>(pointObject);

        if (transitionStep == nullptr)
        {
            Debug::warn("CameraTransitionEvent on '%s' found camera point '%s' without CameraTransitionStep.", GameObjectAPI::getName(getOwner()), pointName);
        }

        m_transitionSteps.push_back(transitionStep);
    }
}

Transform* CameraTransitionEvent::findCameraPointsRoot() const
{
    GameObject* owner = getOwner();
    Transform* ownerTransform = GameObjectAPI::getTransform(owner);

    return TransformAPI::findChildByName(ownerTransform, "CameraPoints");
}

CameraTransitionController* CameraTransitionEvent::findCameraTransitionController() const
{
    GameObject* defaultCamera = SceneAPI::getDefaultCameraGameObject();
    if (defaultCamera == nullptr)
    {
        return nullptr;
    }

    return GameObjectAPI::findScript<CameraTransitionController>(defaultCamera);
}

IMPLEMENT_SCRIPT(CameraTransitionEvent)