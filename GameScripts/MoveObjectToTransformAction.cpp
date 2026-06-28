#include "pch.h"
#include "MoveObjectToTransformAction.h"

#include "CameraTransitionController.h"
#include "CameraTransitionStep.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(MoveObjectToTransformAction, CameraTransitionStepAction,
    SERIALIZED_COMPONENT_REF(m_objectToMove, "Object To Move", ComponentType::TRANSFORM),
    SERIALIZED_COMPONENT_REF(m_targetTransform, "Target Transform", ComponentType::TRANSFORM),
    SERIALIZED_FLOAT(m_moveDuration, "Move Duration", 0.0f, 20.0f, 0.05f),

    FIELD_GROUP_COLLAPSE("Animations",
        SERIALIZED_BOOL(m_playAnimationWhileMoving, "Play Animation While Moving"),
        SERIALIZED_STRING(m_movingClipName, "Moving Clip Name"),
        SERIALIZED_BOOL(m_clearAnimationOnFinish, "Clear Animation On Finish")
    )
)

MoveObjectToTransformAction::MoveObjectToTransformAction(GameObject* owner)
    : CameraTransitionStepAction(owner)
{
}

void MoveObjectToTransformAction::Update()
{
    CameraTransitionStepAction::Update();

    if (!m_isMoving)
    {
        return;
    }

    const float dt = Time::getDeltaTime();
    updateMove(dt);
}

void MoveObjectToTransformAction::executeAction(CameraTransitionController* controller, CameraTransitionStep* step)
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    Transform* targetTransform = m_targetTransform.getReferencedComponent();

    if (objectToMove == nullptr)
    {
        Debug::warn("MoveObjectToTransformAction on '%s' has no valid Object To Move assigned.", GameObjectAPI::getName(getOwner()));
        return;
    }

    if (targetTransform == nullptr)
    {
        Debug::warn("MoveObjectToTransformAction on '%s' has no valid Target Transform assigned.", GameObjectAPI::getName(getOwner()));
        return;
    }

    startMove();
}

void MoveObjectToTransformAction::startMove()
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    Transform* targetTransform = m_targetTransform.getReferencedComponent();

    if (objectToMove == nullptr || targetTransform == nullptr)
    {
        return;
    }

    m_isMoving = true;
    m_timer = 0.0f;

    m_startPosition = TransformAPI::getGlobalPosition(objectToMove);
    m_startRotation = TransformAPI::getGlobalEulerDegrees(objectToMove);

    m_targetPosition = TransformAPI::getGlobalPosition(targetTransform);
    m_targetRotation = TransformAPI::getGlobalEulerDegrees(targetTransform);

    if (m_playAnimationWhileMoving)
    {
        playOverrideClip(m_movingClipName, 0.15, true);
    }

    if (m_moveDuration <= 0.0001f)
    {
        finishMove();
    }
}

void MoveObjectToTransformAction::updateMove(float dt)
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

    const Vector3 newPosition = MathAPI::lerp(m_startPosition, m_targetPosition, alpha);
    const Vector3 newRotation = MathAPI::lerp(m_startRotation, m_targetRotation, alpha);

    TransformAPI::setGlobalPosition(objectToMove, newPosition);
    TransformAPI::setGlobalRotationEuler(objectToMove, newRotation);

    if (m_timer >= m_moveDuration)
    {
        finishMove();
    }
}

void MoveObjectToTransformAction::finishMove()
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        m_isMoving = false;
        return;
    }

    TransformAPI::setGlobalPosition(objectToMove, m_targetPosition);
    TransformAPI::setGlobalRotationEuler(objectToMove, m_targetRotation);

    if (m_clearAnimationOnFinish)
    {
        clearOverrideClip(0.15);
    }

    m_isMoving = false;
    m_timer = 0.0f;
}

void MoveObjectToTransformAction::playOverrideClip(const std::string& clipName, float transitionTimeSeconds, bool loop)
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
        Debug::warn("MoveObjectToTransformAction on '%s' could not find AnimationComponent on moved object.", GameObjectAPI::getName(getOwner()));
        return;
    }

    const bool success = AnimationAPI::playOverrideClip(animation, clipName.c_str(), transitionTimeSeconds, loop);

    if (!success)
    {
        Debug::warn("MoveObjectToTransformAction on '%s' failed to play override clip '%s'.", GameObjectAPI::getName(getOwner()), clipName.c_str());
    }
}

void MoveObjectToTransformAction::clearOverrideClip(float transitionTimeSeconds)
{
    Transform* objectToMove = m_objectToMove.getReferencedComponent();
    if (objectToMove == nullptr)
    {
        return;
    }

    GameObject* object = ComponentAPI::getOwner(objectToMove);

    AnimationComponent* animation = AnimationAPI::getAnimationComponent(object);
    if (animation == nullptr)
    {
        return;
    }

    AnimationAPI::clearOverrideClip(animation, transitionTimeSeconds);
}

IMPLEMENT_SCRIPT(MoveObjectToTransformAction)