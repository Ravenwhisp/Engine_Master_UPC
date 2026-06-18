#pragma once

#include "LyrielAbilityBase.h"
#include <vector>

class Damageable;
class LyrielUI;

class LyrielArrowVolley : public LyrielAbilityBase
{
    DECLARE_SCRIPT(LyrielArrowVolley)

public:
    explicit LyrielArrowVolley(GameObject* owner);

    void Start() override;
    void Update() override;
    void drawGizmo() override;

protected:
	void startAbility() override;

    void onAttackWindowUpdate() override;
    void onAttackWindowFinished() override;

    float getCooldown() const override;

private:
    void beginAim();
    void updateAim();
    void releaseAimAndCast();

    bool canCast() const;

    Vector3 computeAimDirection() const;
    bool isAimStickValid(const Vector3& direction) const;

    void collectEnemiesInCone(const Vector3& origin, const Vector3& forward, std::vector<Damageable*>& outTargets);
    bool applyVolleyDamage(const std::vector<Damageable*>& targets);
    void spawnVolleyArrows(const Vector3& origin, const Vector3& forward);

    void drawAimPreview(const Vector3& origin, const Vector3& forward) const;

private:
    LyrielUI* m_lyrielUI = nullptr;

    bool m_isAiming = false;
    Vector3 m_currentAimDirection = Vector3::Zero;
    Vector3 m_attackFacingDirection = Vector3::Zero;
};