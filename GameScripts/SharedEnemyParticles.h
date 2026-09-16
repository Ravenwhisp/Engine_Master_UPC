#pragma once

#include "ScriptAPI.h"

class SharedEnemyParticles final : public Script
{
    DECLARE_SCRIPT(SharedEnemyParticles)

public:
    explicit SharedEnemyParticles(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

    void notifyMoving();
    void stopMovementParticle();

private:
    void startMovementParticle();
    void updateMovementParticle();
    void ensureMovementParticle();

    Vector3 getMovementParticlePosition() const;
    Vector3 getOwnerRotation() const;

private:
    PrefabRef m_movementParticlePrefab;
    std::string m_movementParticlePath = "Assets/Prefabs/Particles/VFXRemake/Enemies/PS_RunEnemy.prefab";

    float m_movementParticleYOffset = 0.05f;
    float m_movementParticleForwardOffset = -0.35f;
    float m_movementNotificationTimeout = 0.1f;

    Transform* m_ownerTransform = nullptr;

    GameObject* m_movementParticle = nullptr;
    Transform* m_movementParticleTransform = nullptr;

    float m_movementNotificationTimer = 0.0f;
    bool m_movementParticleActive = false;
};