#pragma once

#include "EnemyBaseController.h"

class EnemyDetectionAggro;
class ArcherAttackConfig;
class Transform;

class RangedEnemyController : public EnemyBaseController
{
    DECLARE_SCRIPT(RangedEnemyController)

public:
    explicit RangedEnemyController(GameObject* owner);
    FieldList getExposedFields() const override;

    void Start() override;
    void Update() override;

    bool isTargetInAttackRange() const;

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

protected:
    Transform* acquireCurrentTarget() override;
    bool isTargetDowned(Transform* target) const override;

private:
    EnemyDetectionAggro* m_enemyDetectionAggro = nullptr;
    AssetReference<ArcherAttackConfig> m_attackConfig;

    float m_somersaultCooldownTimer = 0.0f;
    float m_arrowBarrageCooldownTimer = 0.0f;
};