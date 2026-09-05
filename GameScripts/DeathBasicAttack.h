#pragma once

#include "DeathAbilityBase.h"

class DeathUI;
class DeathParticles;

class DeathBasicAttack : public DeathAbilityBase
{
    DECLARE_SCRIPT(DeathBasicAttack)

public:
    explicit DeathBasicAttack(GameObject* owner);

    void Start()      override;
    void Update()     override;
    void drawGizmo()  override;

protected:
    void onAttackWindowUpdate()   override;
    void onAttackWindowFinished() override;
    void onHitFrame()             override;

	bool canStartSpecificAbility() const override;

    int getAttackVariant() const override;

    float getCooldown() const override;

private:
    void startAbility() override;
    bool isTargetInRange(GameObject* target) const;
    void snapFaceTarget(GameObject* target);
    void faceTarget(GameObject* target);
	void dealDamageToTarget(GameObject* target) const;

    void updateUI() override;

private:
    DeathUI* m_deathUI = nullptr;

    GameObject* m_attackFacingTarget = nullptr;

    DeathParticles* m_particles = nullptr;

    int m_comboVariant = 0;
};
