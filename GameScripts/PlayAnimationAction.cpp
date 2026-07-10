#include "pch.h"
#include "PlayAnimationAction.h"

#include "CameraTransitionController.h"
#include "CameraTransitionStep.h"

IMPLEMENT_SCRIPT_FIELDS_INHERITED(PlayAnimationAction, CameraTransitionStepAction,
    SERIALIZED_COMPONENT_REF(m_animationTarget, "Animation Target", ComponentType::TRANSFORM),
    SERIALIZED_STRING(m_clipName, "Clip Name"),
    SERIALIZED_BOOL(m_loop, "Loop"),
    SERIALIZED_FLOAT(m_duration, "Duration", 0.0f, 20.0f, 0.05f)
)

PlayAnimationAction::PlayAnimationAction(GameObject* owner)
    : CameraTransitionStepAction(owner)
{
}

void PlayAnimationAction::Update()
{
    CameraTransitionStepAction::Update();

    if (!m_waitingToClear)
    {
        return;
    }

    updateClearTimer(Time::getDeltaTime());
}

void PlayAnimationAction::executeAction(CameraTransitionController* controller, CameraTransitionStep* step)
{
    if (m_clipName.empty())
    {
        return;
    }

    AnimationComponent* animation = findAnimationComponent();
    if (animation == nullptr)
    {
        Debug::warn("PlayAnimationAction on '%s' could not find AnimationComponent on target.", GameObjectAPI::getName(getOwner()));
        return;
    }

    const bool success = AnimationAPI::playOverrideClip(animation, m_clipName.c_str(), 0.15, m_loop);

    if (!success)
    {
        Debug::warn("PlayAnimationAction on '%s' failed to play animation clip '%s'.", GameObjectAPI::getName(getOwner()), m_clipName.c_str());
        return;
    }

    m_waitingToClear = true;
    m_timer = 0.0f;

    if (m_duration <= 0.0001f)
    {
        AnimationAPI::clearOverrideClip(animation, 0.15);
        m_waitingToClear = false;
    }
}

void PlayAnimationAction::updateClearTimer(float dt)
{
    m_timer += dt;

    if (m_timer < m_duration)
    {
        return;
    }

    AnimationComponent* animation = findAnimationComponent();
    if (animation != nullptr)
    {
        AnimationAPI::clearOverrideClip(animation, 0.15);
    }

    m_waitingToClear = false;
    m_timer = 0.0f;
}

AnimationComponent* PlayAnimationAction::findAnimationComponent() const
{
    Transform* targetTransform = m_animationTarget.getReferencedComponent();
    if (targetTransform == nullptr)
    {
        return nullptr;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);

    return AnimationAPI::getAnimationComponent(targetObject);
}

IMPLEMENT_SCRIPT(PlayAnimationAction)