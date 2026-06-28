#pragma once

#include "ScriptAPI.h"
#include "CameraTransitionStepAction.h"

class Transform;

class MoveObjectToTransformAction : public CameraTransitionStepAction
{
    DECLARE_SCRIPT(MoveObjectToTransformAction)

public:
    explicit MoveObjectToTransformAction(GameObject* owner);

    void Update() override;

    ScriptFieldList getExposedFields() const override;

private:
    void executeAction(CameraTransitionController* controller, CameraTransitionStep* step) override;

    void startMove();
    void updateMove(float dt);
    void finishMove();

    void playOverrideClip(const std::string& clipName, float transitionTimeSeconds, bool loop);
    void clearOverrideClip(float transitionTimeSeconds);

public:
    ScriptComponentRef<Transform> m_objectToMove;
    ScriptComponentRef<Transform> m_targetTransform;

    float m_moveDuration = 1.0f;

    bool m_playAnimationWhileMoving = false;
    std::string m_movingClipName = "Walk";
    bool m_clearAnimationOnFinish = false;

private:
    bool m_isMoving = false;
    float m_timer = 0.0f;

    Vector3 m_startPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_startRotation = Vector3(0.0f, 0.0f, 0.0f);

    Vector3 m_targetPosition = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 m_targetRotation = Vector3(0.0f, 0.0f, 0.0f);
};