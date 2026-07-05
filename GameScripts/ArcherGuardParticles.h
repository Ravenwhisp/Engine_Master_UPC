#pragma once
#include "ScriptAPI.h"
#include <string>
#include <vector>

class ArcherGuardParticles : public Script
{
    DECLARE_SCRIPT(ArcherGuardParticles)
public:
    explicit ArcherGuardParticles(GameObject* owner);
    void Start() override;
    ScriptFieldList getExposedFields() const override;

    std::string m_trailPrefab  = "Assets/Prefabs/Particles/Archer/ArcherGuardBasicShot.prefab";
    std::string m_volleyPrefab = "Assets/Prefabs/Particles/Archer/ArcherGuardVolley.prefab";
    std::string m_chargePrefab = "Assets/Prefabs/Particles/Archer/ArcherGuardCharge.prefab";
    std::string m_arrowPrefab  = "Assets/Prefabs/Projectiles/ArcherArrow.prefab";

    // Basic attack trail — called by ArcherArrowShooter
    void spawnBasicAttackTrail(const Vector3& pos);
    void syncBasicAttackTrail(const Vector3& pos, const Vector3& eulerDeg);
    void stopBasicAttackTrail();

    // Barrage — called by ArcherArrowBarrageState
    void spawnBarrageArrows(const Vector3& impactPos, float landDelay);
    void spawnImpactParticle(const Vector3& impactPos);
    void stopBarrageArrows();

    // Charge/somersault — called by ArcherSomersaultState
    void startChargeParticle();
    void updateChargeParticle();
    void stopChargeParticle();

private:
    GameObject*              m_trailGO          = nullptr;
    std::vector<GameObject*> m_barrageArrows;
    GameObject*              m_chargeParticleGO = nullptr;
};
