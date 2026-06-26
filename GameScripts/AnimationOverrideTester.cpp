#include "pch.h"
#include "AnimationOverrideTester.h"

IMPLEMENT_SCRIPT_FIELDS(AnimationOverrideTester,
    SERIALIZED_COMPONENT_REF(m_target, "Target", ComponentType::TRANSFORM),
    SERIALIZED_STRING(m_overrideClipName, "Override Clip Name"),
    SERIALIZED_BOOL(m_loopOverrideClip, "Loop Override Clip")
)

AnimationOverrideTester::AnimationOverrideTester(GameObject* owner)
    : Script(owner)
{
}

void AnimationOverrideTester::Update()
{
    AnimationComponent* animation = getTargetAnimation();
    if (animation == nullptr)
    {
        return;
    }

    if (Input::isKeyDown(KeyCode::Num1))
    {
        const char* activeStateBefore = AnimationAPI::getActiveStateName(animation);

        const bool success = AnimationAPI::playOverrideClip(animation, m_overrideClipName.c_str(), 0.15, m_loopOverrideClip);

        Debug::log("[AnimationOverrideTester] Play override clip '%s'. Success: %s. Active state before: '%s', after: '%s'", m_overrideClipName.c_str(), success ? "true" : "false", activeStateBefore, AnimationAPI::getActiveStateName(animation));
    }

    if (Input::isKeyDown(KeyCode::Num2))
    {
        AnimationAPI::clearOverrideClip(animation, 0.15);

        Debug::log("[AnimationOverrideTester] Clear override clip. Active state is still: '%s'", AnimationAPI::getActiveStateName(animation)
        );
    }
}

AnimationComponent* AnimationOverrideTester::getTargetAnimation() const
{
    Transform* targetTransform = m_target.getReferencedComponent();
    if (targetTransform == nullptr)
    {
        return nullptr;
    }

    GameObject* targetObject = ComponentAPI::getOwner(targetTransform);
    if (targetObject == nullptr)
    {
        return nullptr;
    }

    return AnimationAPI::getAnimationComponent(targetObject);
}

IMPLEMENT_SCRIPT(AnimationOverrideTester)