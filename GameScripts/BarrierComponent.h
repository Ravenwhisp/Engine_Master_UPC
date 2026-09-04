#pragma once

#include "ScriptAPI.h"
#include <vector>

class Transform2D;

struct BarrierResult
{
	bool  blocked = false;
	float damageToApply = 0.0f;
};

class BarrierComponent final : public Script
{
	DECLARE_SCRIPT(BarrierComponent)

public:
	explicit BarrierComponent(GameObject* owner);

	void Start() override;
	void Update() override;

	FieldList getExposedFields() const override;

	bool hasActiveBarriers() const { return m_nextBarrierIndex < m_barriers.size(); }
	size_t getRemainingBarrierCount() const { return m_barriers.size() - m_nextBarrierIndex; }
	bool hasActiveBarrierAt(float hpPercent) const;

	BarrierResult processBarrierDamage(float incomingDamage, float currentHp, float maxHp, bool shadowMarkExploited);
	void setBarrierUIAlpha(float alpha);

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
		bool  broken;
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
	float getNextBarrierAbsoluteHp(float maxHp) const;
	void breakNextBarrier();

	void processPendingDestruction();

	std::vector<Barrier> m_barriers;
	std::vector<BarrierUI> m_barrierUIs;
	size_t m_nextBarrierIndex = 0;
	int m_pendingDestroyBarrierIndex = -1;
};
