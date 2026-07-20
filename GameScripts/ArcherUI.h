#pragma once

#include "ScriptAPI.h"
#include "Transform2D.h"

class ArcherUI : public Script
{
	DECLARE_SCRIPT(ArcherUI)

public:
	explicit ArcherUI(GameObject* owner);

	void Start() override;

	FieldList getExposedFields() const override;

public:
	void setupArrowBarrageUI(float radius);
	void updateArrowBarrageUI(float stateTimer, const Vector3& impactPosition, float throwTime, float landDelay, float totalDuration);
	void hideArrowBarrageUI();

private:
	ComponentRef<Transform> m_arrowBarrageUICanvas;
	ComponentRef<Transform2D> m_arrowBarrageUIContainer;
	ComponentRef<Transform2D> m_arrowBarrageUIGlow;

	Transform* m_arrowBarrageUICanvasTransform = nullptr;
	Transform2D* m_arrowBarrageUIContainerTransform2D = nullptr;
	Transform2D* m_arrowBarrageUIGlowTransform2D = nullptr;
};