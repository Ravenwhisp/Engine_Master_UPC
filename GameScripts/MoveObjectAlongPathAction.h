#pragma once

#include "ScriptAPI.h"
#include "CameraTransitionStepAction.h"

#include <string>
#include <vector>

class Transform;
class AnimationComponent;

class MoveObjectAlongPathAction : public CameraTransitionStepAction
{
    DECLARE_SCRIPT(MoveObjectAlongPathAction)

public:
    explicit MoveObjectAlongPathAction(GameObject* owner);

    void Update() override;

    ScriptFieldList getExposedFields() const override;

private:
    void executeAction(CameraTransitionController* controller, CameraTransitionStep* step) override;

    void collectPathPoints();

    void startMove();
    void updateMove(float dt);
    void finishMove();

    void calculatePathLengths();

    Vector3 evaluatePathByDistance(float normalizedDistance) const;
    Vector3 evaluatePathSegment(int segmentIndex, float localAlpha) const;

    void updateFacingDirection(const Vector3& previousPosition, const Vector3& newPosition);

    void playOverrideClip(const std::string& clipName, float transitionTimeSeconds, bool loop);
    void clearOverrideClip(float transitionTimeSeconds);

public:
    ScriptComponentRef<Transform> m_objectToMove;
    ScriptComponentRef<Transform> m_pathRoot;

    float m_moveDuration = 3.0f;
    bool m_faceMovementDirection = true;

    bool m_playAnimationWhileMoving = false;
    std::string m_movingClipName = "Walk";
    bool m_clearAnimationOnFinish = true;

private:
    bool m_isMoving = false;
    float m_timer = 0.0f;

    std::vector<Transform*> m_pathPoints;

    Vector3 m_previousPosition = Vector3(0.0f, 0.0f, 0.0f);

    std::vector<float> m_segmentLengths;
    std::vector<float> m_accumulatedLengths;
    float m_totalPathLength = 0.0f;
};