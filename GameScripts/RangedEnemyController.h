#pragma once

#include "ScriptAPI.h"

class EnemyDetectionAggro;
class ArcherAttackConfig;
class Transform;

class RangedEnemyController : public Script
{
    DECLARE_SCRIPT(RangedEnemyController)

public:
    explicit RangedEnemyController(GameObject* owner);

    void Start() override;
    void Update() override;

    FieldList getExposedFields() const override;

    Transform* getTarget() const { return m_target; }
    bool hasTarget() const { return m_target != nullptr; }

    void updateCurrentTarget();

    float getDistanceToTarget() const;
    bool isTargetInAttackRange() const;

    bool moveTowardsTarget();
    void faceTarget();
    void clearPath();

    bool isDead() const;
    bool trySendDeathTrigger(AnimationComponent* animation);

    // Somersault helpers
    bool playerInSomersaultRange() const;

    bool isSomersaultReady() const;
    void consumeSomersaultCooldown();
    void updateSomersaultCooldown(float dt);

    Vector3 getDirectionAwayFromClosestPlayer() const;

    // Arrow Barrage helpers
    bool isArrowBarrageReady() const;
    void consumeArrowBarrageCooldown();
    void updateArrowBarrageCooldown(float dt);

    bool isTargetInArrowBarrageRange() const;

private:
    bool rebuildPathToTarget();
    void rotateTowardsDirection(const Vector3& direction);

public:
    float m_moveSpeed = 3.5f;
    float m_pathPointReachDistance = 0.25f;
    float m_repathInterval = 0.5f;
    float m_turnSpeedDegrees = 360.0f;

private:
    EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
    ArcherAttackConfig* m_attackConfig = nullptr;

    Transform* m_target = nullptr;

    std::vector<Vector3> m_path;
    size_t m_currentPathIndex = 0;
    bool m_hasPath = false;
    float m_repathTimer = 0.0f;
    Vector3 m_lastTargetPosition = Vector3::Zero;

    bool m_deathTriggerSent = false;

    float m_somersaultCooldownTimer = 0.0f;
    float m_arrowBarrageCooldownTimer = 0.0f;
};