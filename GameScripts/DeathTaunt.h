#pragma once
#include "DeathAbilityBase.h"

class EnemyDetectionAggro;
class PlayerRotation;
class DeathUI;
class DeathParticles;
class EnemyForcedMovement;

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
    void onAttackWindowFinished() override;

    float getCooldown() const override;

private:
    void beginAim();
    void updateAim();
    void releaseAimAndCast();

    void updateImpactDelay();
    void resolveImpact();

    std::vector<GameObject*> collectEnemiesInCone(const Vector3& origin, const Vector3& direction) const;
    void applyTauntEffects(GameObject* enemy, Transform* deathTransform) const;

    Vector3 calculatePullDestination(GameObject* enemy) const;

    bool isEnemyInsideTauntCone(GameObject* enemy, const Vector3& ownerPosition, const Vector3& ownerForward) const;

    Vector3 computeAimDirection() const;
    void faceDirection(const Vector3& direction);
    bool isAimStickValid(const Vector3& direction) const;

private:
    enum class TauntState
    {
        Idle,
        Aiming,
        WaitingForImpact
    };

    PlayerRotation* m_playerRotation = nullptr;
    DeathUI* m_deathUI = nullptr;
    DeathParticles* m_deathParticles = nullptr;

    TauntState m_tauntState = TauntState::Idle;

    Vector3 m_currentAimDirection = Vector3::Zero;
    Vector3 m_castOrigin = Vector3::Zero;
    Vector3 m_castDirection = Vector3::Zero;

    float m_impactDelayTimer = 0.0f;
    float m_debugConeTimer = 0.0f;
};
