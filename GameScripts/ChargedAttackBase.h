#pragma once

#include "AbilityBase.h"

class PlayerMovement;
class EnemyBaseController;

class ChargedAttackBase : public AbilityBase
{
public:
    explicit ChargedAttackBase(GameObject* owner);

    void Start() override;

protected:
    void applyChargingMovementSlowdown(float slowdownPercentage);
    void resetChargingMovementSlowdown();

    void tryStunTarget(GameObject* target, bool isMaxCharge, bool stunEnabled, float stunDuration) const;

private:
    PlayerMovement* m_playerMovement = nullptr;
};