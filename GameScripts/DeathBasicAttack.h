#pragma once

#include "DeathAbilityBase.h"

class DeathUI;
class DeathParticles;

class DeathBasicAttack : public DeathAbilityBase
{
    DECLARE_SCRIPT(DeathBasicAttack)

public:
    explicit DeathBasicAttack(GameObject* owner);

    FieldList getExposedFields() const override;

    void Start()      override;
    void Update()     override;
    void drawGizmo()  override;

protected:
    void onAttackWindowUpdate()   override;
    void onAttackWindowFinished() override;

	bool canStartSpecificAbility() const override;

    float getCooldown() const override;

private:
    void startAbility() override;
    void snapFaceTarget(GameObject* target);
    void faceTarget(GameObject* target);
	void dealDamageToTarget(GameObject* target) const;

    void updateUI() override;

private:
    DeathUI* m_deathUI = nullptr;

    GameObject* m_attackFacingTarget = nullptr;

    DeathParticles* m_particles = nullptr;
};
