#include "pch.h"
#include "CameraTransitionEvent.h"

#include "GameplayEventTrigger.h"
#include "CameraTransitionController.h"

IMPLEMENT_SCRIPT_FIELDS(CameraTransitionEvent,
    SERIALIZED_FLOAT(m_pathDuration, "Path Duration", 0.0f, 20.0f, 0.05f),
    SERIALIZED_FLOAT(m_holdDuration, "Hold Duration", 0.0f, 20.0f, 0.05f),
    SERIALIZED_FLOAT(m_returnDuration, "Return Duration", 0.0f, 20.0f, 0.05f),
    SERIALIZED_BOOL(m_useFovTransition, "Use FOV Transition"),
    SERIALIZED_FLOAT(m_targetFov, "Target FOV", 5.0f, 120.0f, 0.1f)
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

Transform* CameraTransitionEvent::getTargetPoint(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_targetPoints.size()))
    {
        return nullptr;
    }

    return m_targetPoints[index];
}

void CameraTransitionEvent::findTargetPoints()
{
    m_targetPoints.clear();

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