#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"

class ArcherUI : public Script
{
	DECLARE_SCRIPT(ArcherUI)

public:
	explicit ArcherUI(GameObject* owner);

	void Start() override;

	ScriptFieldList getExposedFields() const override;

public:
	void setupArrowBarrageUI(float radius);
	void updateArrowBarrageUI(float stateTimer, const Vector3& impactPosition, float throwTime, float landDelay, float totalDuration);
	void hideArrowBarrageUI();

private:
	ScriptComponentRef<Transform> m_arrowBarrageUICanvas;
	ScriptComponentRef<Transform2D> m_arrowBarrageUIContainer;
	ScriptComponentRef<Transform2D> m_arrowBarrageUIGlow;

	Transform* m_arrowBarrageUICanvasTransform = nullptr;
	Transform2D* m_arrowBarrageUIContainerTransform2D = nullptr;
	Transform2D* m_arrowBarrageUIGlowTransform2D = nullptr;
};