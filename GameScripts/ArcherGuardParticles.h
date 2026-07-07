#pragma once
#include "ScriptAPI.h"
#include <vector>

class ArcherGuardParticles : public Script
{
    DECLARE_SCRIPT(ArcherGuardParticles)
public:
    explicit ArcherGuardParticles(GameObject* owner);
    void Start() override;
    ScriptFieldList getExposedFields() const override;

    PrefabRef m_trailPrefab;
    PrefabRef m_volleyPrefab;
    PrefabRef m_chargePrefab;
    PrefabRef m_arrowPrefab;

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
