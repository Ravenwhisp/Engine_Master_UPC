#pragma once

#include "EnemyDamageable.h"
#include <vector>

class BarrierEnemyDamageable : public EnemyDamageable
{
    DECLARE_SCRIPT(BarrierEnemyDamageable)

public:
    explicit BarrierEnemyDamageable(GameObject* owner);

    void Start() override;
    FieldList getExposedFields() const override;

    void takeDamage(float amount) override;
    void takeDamage(const HitContext& ctx) override;
    void kill() override;

    bool hasActiveBarriers() const { return m_nextBarrierIndex < m_barriers.size(); }
    size_t getRemainingBarrierCount() const { return m_barriers.size() - m_nextBarrierIndex; }

public:
    std::vector<float> m_barriersThresholds;
    PrefabRef m_barrierPrefab;

    float m_minPos = -80.0f;
    float m_maxPos = 80.0f;
    float m_barrierUIHeight = 0.0f;

private:
    struct Barrier
    {
        float hpPercent;
        bool broken;
    };

    struct BarrierUI
    {
        GameObject* gameObject = nullptr;
        Transform2D* transform2D = nullptr;
        float hpPercent;
    };

    void buildBarriers();
    void instantiateBarrierUIs();
    void destroyBrokenBarrierUI(size_t index);
    float getNextBarrierAbsoluteHp() const;
    void breakNextBarrier();

    std::vector<Barrier> m_barriers;
    std::vector<BarrierUI> m_barrierUIs;
    size_t m_nextBarrierIndex = 0;

protected:
    void setHealthBarAlpha(float alpha) override;
};
