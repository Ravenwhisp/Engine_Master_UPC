#pragma once

#include "ScriptAPI.h"

class AnimationComponent;
class Transform;
class EnemySound;
class EnemyBaseAttackConfig;

class EnemyBaseController : public Script
{
public:
    explicit EnemyBaseController(GameObject* owner);
    virtual ~EnemyBaseController() = default;

    void Start() override;

    FieldList getExposedFields() const override;

    // Target helpers
    virtual void updateCurrentTarget();
    Transform* getCurrentTarget() const { return m_currentTarget; }

    virtual bool hasValidTarget() const;

    // Attack config access (non-pure virtual: returns nullptr by default for controllers without one)
    virtual const EnemyBaseAttackConfig* getAttackConfig() const { return nullptr; }

    float getDistanceToCurrentTarget() const;
    bool isCurrentTargetInRange(float range) const;
    bool isTargetInAttackRange() const;

    // Facing helpers
    void faceCurrentTarget();
    void facePosition(const Vector3& worldPosition);

    // Movement/path helpers
    bool moveTowardsTarget();

    virtual void clearPath();
    virtual void resetRepathTimer();

    // Forced movement helpers
    void setForcedMovementActive(bool active);
    bool isForcedMovementActive() const { return m_isForcedMovementActive; }
    void setForcedMovementBlocked(bool blocked);
    bool isForcedMovementBlocked() const { return m_isForcedMovementBlocked; }

    // Death helpers
    bool isDead() const;
    bool trySendDeathTrigger(AnimationComponent* animation);

    // Recovery helpers
    void setRecoveryDuration(float recoveryDuration);
    float getRecoveryDuration() const { return m_recoveryDuration; }

    // Stunned helpers
    void setStunnedDuration(float stunnedDuration);
    float getStunnedDuration() const { return m_stunnedDuration; }
    void useStun(float duration);
    bool trySendStunTrigger(AnimationComponent* animation);
    bool isStunned() const { return m_isStunned; }
    void clearStun();
    void updateStun(float dt);

private:
    void rotateTowardsDirection(const Vector3& direction);

    bool buildPathToTarget();
    bool followPath();

    // Footstep audio is driven from locomotion: ping the EnemySound each frame we step.
    EnemySound* m_enemySound = nullptr;
    bool m_enemySoundResolved = false;

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

    bool m_isForcedMovementActive = false;
    bool m_isForcedMovementBlocked = false;

    float m_recoveryDuration = 0.75f;
    float m_stunnedDuration = 2.0f;
    float m_stunnedTimer = 0.0f;
    bool m_stunnedTriggerSent = false;
    bool m_isStunned = false;

private:
    static constexpr size_t MAX_PATH_POINTS = 128;
};