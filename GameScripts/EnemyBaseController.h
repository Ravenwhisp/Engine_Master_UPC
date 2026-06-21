#pragma once

#include "ScriptAPI.h"

class AnimationComponent;
class Transform;

class EnemyBaseController : public Script
{
public:
    explicit EnemyBaseController(GameObject* owner);
    virtual ~EnemyBaseController() = default;

    ScriptFieldList getExposedFields() const override;

    // Target helpers
    virtual void updateCurrentTarget();
    Transform* getCurrentTarget() const { return m_currentTarget; }

    virtual bool hasValidTarget() const;

    float getDistanceToCurrentTarget() const;
    bool isCurrentTargetInRange(float range) const;

    // Facing helpers
    void faceCurrentTarget();
    void facePosition(const Vector3& worldPosition);

    // Movement/path helpers
    bool moveTowardsTarget();

    virtual void clearPath();
    virtual void resetRepathTimer();

    // Death helpers
    bool isDead() const;
    bool trySendDeathTrigger(AnimationComponent* animation);

    // Recovery helpers
    void setRecoveryDuration(float recoveryDuration);
    float getRecoveryDuration() const { return m_recoveryDuration; }

    // Stunned helpers
    void setStunnedDuration(float stunnedDuration);
    float getStunnedDuration() const { return m_stunnedDuration; }
    void useStun();
    bool trySendStunTrigger(AnimationComponent* animation);
    bool isStunned() const { return m_isStunned; }
    void clearStun();
    void updateStun(float dt);

private:
    void rotateTowardsDirection(const Vector3& direction);

    bool buildPathToTarget();
    bool followPath();

protected:
    //These we will not need if we Refactor DetectionAggro to have a base class
    virtual Transform* acquireCurrentTarget() = 0;
    virtual bool isTargetDowned(Transform* target) const = 0;

    virtual Vector3 getPathDestination() const;

public:
    int m_enemyType = static_cast<int>(NavAgentProfile::EnemyGround);

    float m_moveSpeed = 3.5f;
    float m_turnSpeedDegrees = 360.0f;
    float m_repathInterval = 0.5f;
    float m_pathPointReachDistance = 0.25f;

    Vector3 m_pathSearchExtents = Vector3(5.0f, 5.0f, 5.0f);

protected:
    Transform* m_currentTarget = nullptr;
    bool m_deathTriggerSent = false;

    std::vector<Vector3> m_path;
    size_t m_currentPathIndex = 0;
    bool m_hasPath = false;
    float m_repathTimer = 0.0f;

    float m_recoveryDuration = 0.75f;
    float m_stunnedDuration = 2.0f;
    float m_stunnedTimer = 0.0f;
    bool m_stunnedTriggerSent = false;
    bool m_isStunned = false;

private:
    static constexpr size_t MAX_PATH_POINTS = 128;
};