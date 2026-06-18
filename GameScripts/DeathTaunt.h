#pragma once
#include "DeathAbilityBase.h"

class EnemyDetectionAggro;
class PlayerRotation;
class DeathUI;

class DeathTaunt : public DeathAbilityBase
{
    DECLARE_SCRIPT(DeathTaunt)

public:
    explicit DeathTaunt(GameObject* owner);

    void Start()  override;
    void Update() override;

    void drawGizmo() override;

protected:
	void startAbility() override;

	bool canStartSpecificAbility() const override;

    float getCooldown() const override;

private:
    void beginAim();
    void updateAim();
    void releaseAimAndCast();

    void applyTauntToEnemiesInCone(const Vector3& ownerForward) const;
    bool isEnemyInsideTauntCone(GameObject* enemy, const Vector3& ownerPosition, const Vector3& ownerForward) const;

    Vector3 computeAimDirection() const;
    void faceDirection(const Vector3& direction);
    bool isAimStickValid(const Vector3& direction) const;

private:
    PlayerRotation* m_playerRotation = nullptr;
    DeathUI* m_deathUI = nullptr;

    float m_debugConeTimer = 0.0f;
    bool m_isAiming = false;
    Vector3 m_currentAimDirection = Vector3::Zero;
};
