#pragma once

#include "DeathAbilityBase.h"

class DeathBasicAttack : public DeathAbilityBase
{
    DECLARE_SCRIPT(DeathBasicAttack)

public:
    explicit DeathBasicAttack(GameObject* owner);

    void Start()      override;
    void Update()     override;
    void drawGizmo()  override;

    ScriptFieldList getExposedFields() const override;

protected:
    void onAttackWindowUpdate()   override;
    void onAttackWindowFinished() override;

	bool canStartSpecificAbility() const override;

private:
    void startAbility() override;
    void snapFaceTarget(GameObject* target);
    void faceTarget(GameObject* target);
	void dealDamageToTarget(GameObject* target) const;

    GameObject* m_attackFacingTarget    = nullptr;

public:
    float m_basicAttackDamage = 20.0f;
    float m_basicAttackRange = 1.5f;
    float m_basicAttackHitAngle = 50.0f;

    float m_attackLockDuration = 0.35f;
    float m_finalHitLockDuration = 0.7f;
};
