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

    float m_attackLockDuration      = 0.35f;
    float m_finalHitLockDuration    = 0.7f;

protected:
    void onAttackWindowUpdate()   override;
    void onAttackWindowFinished() override;

private:
    void tryAttack();
    void snapFaceTarget(GameObject* target);
    void faceTarget(GameObject* target);
    void releaseComboMoveLock();

    GameObject* m_attackFacingTarget    = nullptr;
    bool        m_movementLockedForCombo = false;
};
