#pragma once
#include "ScriptAPI.h"

class ArcherAttackConfig;
class RangedEnemyController;
class AnimationComponent;
class ArcherGuardParticles;

class ArcherArrowShooter : public Script
{
    DECLARE_SCRIPT(ArcherArrowShooter)
public:
    explicit ArcherArrowShooter(GameObject* owner);
    void Start()  override;
    void Update() override;
    ScriptFieldList getExposedFields() const override;

    PrefabRef m_arrowPrefab;

private:
    ArcherAttackConfig*    m_config     = nullptr;
    RangedEnemyController* m_controller = nullptr;
    AnimationComponent*    m_animation  = nullptr;
    ArcherGuardParticles*  m_particles  = nullptr;

    bool        m_inAttack = false;
    float       m_timer    = 0.0f;
    bool        m_fired    = false;
    GameObject* m_arrowGO  = nullptr;
};
