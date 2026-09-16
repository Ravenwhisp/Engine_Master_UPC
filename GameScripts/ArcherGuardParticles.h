#pragma once
#include "ScriptAPI.h"
#include <vector>

class ArcherGuardParticles : public Script
{
    DECLARE_SCRIPT(ArcherGuardParticles)
public:
    explicit ArcherGuardParticles(GameObject* owner);
    void Start() override;
    void Update() override;
    void OnGameStop() override;

    FieldList getExposedFields() const override;

    // Basic attack trail — persistent VFX attached to projectile path
    void spawnBasicAttackTrail(const Vector3& pos);
    void syncBasicAttackTrail(const Vector3& pos, const Vector3& eulerDeg);
    void stopBasicAttackTrail();

    // Basic attack projectile sparks — called by ArcherArrowShooter
    void spawnArrowSparks(const Vector3& pos);
    void syncArrowSparks(const Vector3& pos, const Vector3& eulerDeg);
    void stopArrowSparks();

    // Barrage — called by ArcherArrowBarrageState
    void startBarrageFloorParticle(const Vector3& position);
    void stopBarrageFloorParticle();

    void playBarrageImpactParticle(const Vector3& position);

    // Charge/somersault — called by ArcherSomersaultState
    void startChargeParticle();
    void updateChargeParticle();
    void stopChargeParticle();

public:    
    PrefabRef m_trailPrefab;
    PrefabRef m_barrageFloorPrefab;
    PrefabRef m_barrageImpactPrefab;
    PrefabRef m_somersaultPrefab;
    PrefabRef m_arrowSparksPrefab;

    std::string m_barrageFloorPath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level1/Ranged/PS_ArrowBarrage.prefab";
    std::string m_arrowSparksPath = "Assets/Prefabs/Particles/VFXRemake/Enemies/Enemies_Level1/Ranged/PS_ArcherArrowSparks.prefab";

    float m_barrageFloorYOffset = 0.05f;
    float m_barrageImpactYOffset = 0.05f;
    float m_barrageImpactLifetime = 1.0f;

private:
    void ensureTrailParticle(const Vector3& pos);
    void ensureBarrageFloorParticle(const Vector3& position);
    void ensureSomersaultParticle();
    void ensureArrowSparksParticle(const Vector3& pos);

    GameObject* m_trailGO = nullptr;
    GameObject* m_arrowSparksGO = nullptr;
    GameObject* m_barrageFloorParticle = nullptr;
    GameObject* m_barrageImpactParticle = nullptr;

    float m_barrageImpactTimer = 0.0f;

    GameObject* m_somersaultParticle = nullptr;
    Transform* m_somersaultParticleTransform = nullptr;
};
