#pragma once

#include "ScriptAPI.h"
#include <vector>

class PlayerController;
class CharacterBase;
class DeathSound;
class LyrielSound;

class PlayerTargetController : public Script
{
    DECLARE_SCRIPT(PlayerTargetController)

public:
    explicit PlayerTargetController(GameObject* owner);

    void Start() override;
    void Update() override;
    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

    GameObject* getCurrentTarget() const { return m_currentTarget; }

private:
    void updateTargetsInRange();
    void ensureValidCurrentTarget();

    bool canUpdateTarget() const;
    void updateCurrentTarget();
    void setCurrentTarget(GameObject* newTarget);

    Vector3 computeAimDirection() const;
    bool isAimStickValid(const Vector3& direction) const;

    bool tryComputeTargetScore(GameObject* target, const Vector3& aimDirection, float& outScore) const;
    GameObject* findBestTarget(const Vector3& aimDirection, float& outBestScore) const;
    bool shouldSwitchTarget(GameObject* candidate, const Vector3& aimDirection, float candidateScore) const;

    bool isTargetInRange(GameObject* target) const;
    bool isTargetAlive(GameObject* target) const;

public:
    float m_targetRange = 8.0f;

    float m_targetConeAngle = 50.0f;
    float m_angleWeight = 0.85f;
    float m_distanceWeight = 0.15f;

    float m_switchMargin = 0.15f;
    float m_switchCooldown = 0.25f;

private:
    PlayerController* m_playerController = nullptr;
    CharacterBase* m_character = nullptr;

    GameObject* m_currentTarget = nullptr;
    std::vector<GameObject*> m_targetsInRange;

    DeathSound*  m_deathSound  = nullptr;
    LyrielSound* m_lyrielSound = nullptr;

    float m_switchCooldownTimer = 0.0f;
};