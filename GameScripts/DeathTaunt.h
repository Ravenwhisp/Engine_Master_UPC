#pragma once
#include "DeathAbilityBase.h"

class EnemyDetectionAggro;
class PlayerRotation;

class DeathTaunt : public DeathAbilityBase
{
    DECLARE_SCRIPT(DeathTaunt)

public:
    explicit DeathTaunt(GameObject* owner);

    void Start()  override;
    void Update() override;

    ScriptFieldList getExposedFields() const override;
    void drawGizmo() override;
    void onFieldEdited(const ScriptFieldInfo& field) override;

private:
    void beginAim();
    void updateAim();
    void releaseAimAndCast();

    void applyTauntToEnemiesInCone(const Vector3& ownerForward) const;
    bool isEnemyInsideTauntCone(GameObject* enemy, const Vector3& ownerPosition, const Vector3& ownerForward) const;

    Vector3 computeAimDirection() const;
    Vector3 getFallbackFacingDirection() const;
    void faceDirection(const Vector3& direction);
    bool isAimStickValid(const Vector3& direction) const;

    PlayerRotation* m_playerRotation = nullptr;

    float m_debugConeTimer = 0.0f;
    bool m_isAiming = false;
    Vector3 m_currentAimDirection = Vector3::Zero;

public:
    float m_TauntCooldownSeconds = 8.0f;
    float m_TauntDurationSeconds = 3.0f;
    float m_TauntRange = 2.5f;
    float m_TauntHalfAngleDegrees = 35.0f;
};
