#pragma once

#include "ScriptAPI.h"
#include "ParticleLifecycle.h"

#include <string>

class SkeletonParticles final : public Script
{
    DECLARE_SCRIPT(SkeletonParticles)

public:
    explicit SkeletonParticles(GameObject* owner);

    void Start() override;
    void Update() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

    void startReviveParticle();
    void stopReviveParticle();

    const AssetId& getShieldHitVfxId() const { return m_shieldHitPrefab.m_id; }
    const AssetId& getThirdAttackHitVfxId() const { return m_thirdAttackHitPrefab.m_id; }

private:
    Vector3 getReviveParticlePosition() const;
    Vector3 getOwnerRotation() const;

    void updateReviveParticle();
    void ensureReviveParticle();

private:
    PrefabRef m_reviveParticlePrefab;
    PrefabRef m_shieldHitPrefab;
    PrefabRef m_thirdAttackHitPrefab;

    std::string m_reviveParticlePath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level2/Skeletons/PS_Revive.prefab";
    std::string m_shieldHitPath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level2/Skeletons/PS_ShockwaveSmallParticles.prefab";
    std::string m_thirdAttackHitPath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level2/Skeletons/PS_Skeleton3rdAttack.prefab";

    float m_reviveYOffset = 0.0f;
    float m_reviveForwardOffset = 0.0f;
    float m_reviveDeactivateDelay = 2.0f;

    Transform* m_ownerTransform = nullptr;

    GameObject* m_reviveParticle = nullptr;
    Transform* m_reviveParticleTransform = nullptr;

    ParticleLifecycle::TimedParticleTracker m_timedParticles;
};
