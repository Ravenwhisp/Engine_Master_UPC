#pragma once

#include "LyrielAbilityBase.h"
#include <vector>

class LyrielArrowVolley : public LyrielAbilityBase
{
    DECLARE_SCRIPT(LyrielArrowVolley)

public:
    explicit LyrielArrowVolley(GameObject* owner);

    void Start() override;
    void Update() override;
    void drawGizmo() override;

    ScriptFieldList getExposedFields() const override;

    ScriptComponentRef<Transform> m_AbilityUI;

protected:
	void startAbility() override;

    void onAttackWindowUpdate() override;
    void onAttackWindowFinished() override;

private:
    void beginAim();
    void updateAim();
    void releaseAimAndCast();

    bool canStartAim() const;
    bool canCast() const;

    Vector3 computeAimDirection() const;
    bool isAimStickValid(const Vector3& direction) const;

    void collectEnemiesInCone(const Vector3& origin, const Vector3& forward, std::vector<GameObject*>& outTargets);
    void applyVolleyDamage(const std::vector<GameObject*>& targets);
    void spawnVolleyArrows(const Vector3& origin, const Vector3& forward);

    void drawAimPreview(const Vector3& origin, const Vector3& forward) const;

private:
    bool m_isAiming = false;
    Vector3 m_currentAimDirection = Vector3::Zero;
    Vector3 m_attackFacingDirection = Vector3::Zero;

public:
    float m_volleyDamage = 20.0f;
    float m_volleyRange = 8.0f;
    float m_coneAngleDegrees = 50.0f;

    int m_numVisualArrows = 5;
    float m_arrowSpeed = 18.0f;

    float m_attackLockDuration = 0.2f;
};