#include "pch.h"
#include "MoveObjectAlongPathAction.h"

#include "CameraTransitionController.h"
#include "CameraTransitionStep.h"

#include <cmath>

IMPLEMENT_SCRIPT_FIELDS_INHERITED(MoveObjectAlongPathAction, CameraTransitionStepAction,
    SERIALIZED_COMPONENT_REF(m_objectToMove, "Object To Move", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_pathRoot, "Path Root", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_moveDuration, "Move Duration", 0.0f, 30.0f, 0.05f),
    SERIALIZED_BOOL(m_faceMovementDirection, "Face Movement Direction"),

    FIELD_GROUP_COLLAPSE("Animations",
        SERIALIZED_BOOL(m_playAnimationWhileMoving, "Play Animation While Moving"),
        SERIALIZED_STRING(m_movingClipName, "Moving Clip Name"),
        SERIALIZED_BOOL(m_clearAnimationOnFinish, "Clear Animation On Finish")
    )
)

MoveObjectAlongPathAction::MoveObjectAlongPathAction(GameObject* owner)
    : CameraTransitionStepAction(owner)
{
}

void MoveObjectAlongPathAction::Update()
{
    CameraTransitionStepAction::Update();

    if (!m_isMoving)
    {
        return;
    }

    updateMove(Time::getDeltaTime());
}

void MoveObjectAlongPathAction::executeAction(CameraTransitionController* controller, CameraTransitionStep* step)
{
    if (m_isMoving)
    {
        return;
    }

    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        Debug::warn("MoveObjectAlongPathAction on '%s' has no valid Object To Move assigned.", GameObjectAPI::getName(getOwner()));
        return;
    }

    Transform* pathRoot = m_pathRoot.getReferencedComponent();
    if (pathRoot == nullptr)
    {
        Debug::warn("MoveObjectAlongPathAction on '%s' has no valid Path Root assigned.", GameObjectAPI::getName(getOwner()));
        return;
    }

    collectPathPoints();

    if (m_pathPoints.size() < 2)
    {
        Debug::warn("MoveObjectAlongPathAction on '%s' needs at least 2 path points.", GameObjectAPI::getName(getOwner()));
        return;
    }

    calculatePathLengths();

    startMove();
}

void MoveObjectAlongPathAction::collectPathPoints()
{
    m_pathPoints.clear();

    Transform* pathRoot = m_pathRoot.getReferencedComponent();
    if (pathRoot == nullptr)
    {
        return;
    }

    const int childCount = TransformAPI::getChildCount(pathRoot);

    for (int i = 0; i < childCount; ++i)
    {
        Transform* child = TransformAPI::getChild(pathRoot, i);
        if (child == nullptr)
        {
            continue;
        }

        m_pathPoints.push_back(child);
    }
}

void MoveObjectAlongPathAction::startMove()
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        return;
    }

    m_isMoving = true;
    m_timer = 0.0f;

    m_previousPosition = TransformAPI::getGlobalPosition(objectToMove);

    if (m_playAnimationWhileMoving)
    {
        playOverrideClip(m_movingClipName, 0.15f, true);
    }

    if (m_moveDuration <= 0.0001f)
    {
        finishMove();
    }
}

void MoveObjectAlongPathAction::updateMove(float dt)
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        m_isMoving = false;
        return;
    }

    m_timer += dt;

    float alpha = m_timer / m_moveDuration;
    if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }

    alpha = MathAPI::smoothStep(0.0f, 1.0f, alpha);

    const Vector3 newPosition = evaluatePathByDistance(alpha);

    TransformAPI::setGlobalPosition(objectToMove, newPosition);

    if (m_faceMovementDirection)
    {
        updateFacingDirection(m_previousPosition, newPosition);
    }

    m_previousPosition = newPosition;

    if (m_timer >= m_moveDuration)
    {
        finishMove();
    }
}

void MoveObjectAlongPathAction::finishMove()
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        m_isMoving = false;
        return;
    }

    if (!m_pathPoints.empty())
    {
        Transform* finalPoint = m_pathPoints.back();
        if (finalPoint != nullptr)
        {
            TransformAPI::setGlobalPosition(objectToMove, TransformAPI::getGlobalPosition(finalPoint));
        }
    }

    if (m_clearAnimationOnFinish)
    {
        clearOverrideClip(0.15f);
    }

    m_isMoving = false;
    m_timer = 0.0f;
}

void MoveObjectAlongPathAction::calculatePathLengths()
{
    m_segmentLengths.clear();
    m_accumulatedLengths.clear();
    m_totalPathLength = 0.0f;

    if (m_pathPoints.size() < 2)
    {
        return;
    }

    m_accumulatedLengths.push_back(0.0f);

    for (size_t i = 0; i + 1 < m_pathPoints.size(); ++i)
    {
        const Vector3 current = TransformAPI::getGlobalPosition(m_pathPoints[i]);
        const Vector3 next = TransformAPI::getGlobalPosition(m_pathPoints[i + 1]);

        const Vector3 delta = next - current;
        const float length = delta.Length();

        m_segmentLengths.push_back(length);

        m_totalPathLength += length;
        m_accumulatedLengths.push_back(m_totalPathLength);
    }
}

Vector3 MoveObjectAlongPathAction::evaluatePathSegment(int segmentIndex, float localAlpha) const
{
    const int pointCount = static_cast<int>(m_pathPoints.size());

    const int p1Index = segmentIndex;
    const int p2Index = segmentIndex + 1;

    const int p0Index = p1Index > 0 ? p1Index - 1 : p1Index;
    const int p3Index = p2Index + 1 < pointCount ? p2Index + 1 : p2Index;

    const Vector3 p0 = TransformAPI::getGlobalPosition(m_pathPoints[p0Index]);
    const Vector3 p1 = TransformAPI::getGlobalPosition(m_pathPoints[p1Index]);
    const Vector3 p2 = TransformAPI::getGlobalPosition(m_pathPoints[p2Index]);
    const Vector3 p3 = TransformAPI::getGlobalPosition(m_pathPoints[p3Index]);

    return MathAPI::catmullRom(p0, p1, p2, p3, localAlpha);
}

Vector3 MoveObjectAlongPathAction::evaluatePathByDistance(float normalizedDistance) const
{
    if (m_pathPoints.empty())
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    if (m_pathPoints.size() == 1 || m_totalPathLength <= 0.0001f)
    {
        return TransformAPI::getGlobalPosition(m_pathPoints[0]);
    }

    float targetDistance = normalizedDistance * m_totalPathLength;

    if (targetDistance <= 0.0f)
    {
        return TransformAPI::getGlobalPosition(m_pathPoints.front());
    }

    if (targetDistance >= m_totalPathLength)
    {
        return TransformAPI::getGlobalPosition(m_pathPoints.back());
    }

    int segmentIndex = 0;

    for (int i = 0; i < static_cast<int>(m_segmentLengths.size()); ++i)
    {
        const float segmentStartDistance = m_accumulatedLengths[i];
        const float segmentEndDistance = m_accumulatedLengths[i + 1];

        if (targetDistance >= segmentStartDistance && targetDistance <= segmentEndDistance)
        {
            segmentIndex = i;
            break;
        }
    }

    const float segmentStartDistance = m_accumulatedLengths[segmentIndex];
    const float segmentLength = m_segmentLengths[segmentIndex];

    if (segmentLength <= 0.0001f)
    {
        return TransformAPI::getGlobalPosition(m_pathPoints[segmentIndex]);
    }

    const float localAlpha = (targetDistance - segmentStartDistance) / segmentLength;

    return evaluatePathSegment(segmentIndex, localAlpha);
}

void MoveObjectAlongPathAction::updateFacingDirection(const Vector3& previousPosition, const Vector3& newPosition)
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        return;
    }

    const Vector3 direction = newPosition - previousPosition;

    const float lengthSq = direction.x * direction.x + direction.z * direction.z;
    if (lengthSq <= 0.0001f)
    {
        return;
    }

    const float yawRad = std::atan2(direction.x, direction.z);
    const float yawDeg = yawRad * (180.0f / MathAPI::PI);

    Vector3 currentEuler = TransformAPI::getGlobalEulerDegrees(objectToMove);
    currentEuler.y = yawDeg;

    TransformAPI::setGlobalRotationEuler(objectToMove, currentEuler);
}

void MoveObjectAlongPathAction::playOverrideClip(const std::string& clipName, float transitionTimeSeconds, bool loop)
{
    if (clipName.empty())
    {
        return;
    }

    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        return;
    }

    GameObject* object = ComponentAPI::getOwner(objectToMove);

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(object);
    if (animation == nullptr)
    {
        Debug::warn("MoveObjectAlongPathAction on '%s' could not find AnimationComponent on moved object.", GameObjectAPI::getName(getOwner()));
        return;
    }

    const bool success = AnimationAPI::playOverrideClip(animation, clipName.c_str(), transitionTimeSeconds, loop);

    if (!success)
    {
        Debug::warn("MoveObjectAlongPathAction on '%s' failed to play override clip '%s'.", GameObjectAPI::getName(getOwner()), clipName.c_str());
    }
}

void MoveObjectAlongPathAction::clearOverrideClip(float transitionTimeSeconds)
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        return;
    }

    GameObject* object = ComponentAPI::getOwner(objectToMove);
    if (object == nullptr)
    {
        return;
    }

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(object);
    if (animation == nullptr)
    {
        return;
    }

    AnimationAPI::clearOverrideClip(animation, transitionTimeSeconds);
}

IMPLEMENT_SCRIPT(MoveObjectAlongPathAction)