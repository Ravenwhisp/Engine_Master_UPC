#pragma once

#include "ScriptAPI.h"
#include "CameraTransitionStepAction.h"

class Transform;
class AnimationComponent;

class PlayAnimationAction : public CameraTransitionStepAction
{
    DECLARE_SCRIPT(PlayAnimationAction)

public:
    explicit PlayAnimationAction(GameObject* owner);

    void Update() override;

    ScriptFieldList getExposedFields() const override;

private:
    void executeAction(CameraTransitionController* controller, CameraTransitionStep* step) override;
    void updateClearTimer(float dt);

    AnimationComponent* findAnimationComponent() const;

public:
    ScriptComponentRef<Transform> m_animationTarget;

    std::string m_clipName = "";
    bool m_loop = false;

    float m_duration = 1.0f;

private:
    bool m_waitingToClear = false;
    float m_timer = 0.0f;
};