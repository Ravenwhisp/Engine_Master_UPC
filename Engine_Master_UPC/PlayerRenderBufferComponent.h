#pragma once
#include "Component.h"

class PlayerRenderBufferComponent : public Component
{
public:
	struct DamageHighlightData
	{
		Vector3 centerColor = Vector3::One;
		Vector3 rimColor = Vector3::One;
		float rimIntensity = 1;
	};

	struct PlayerRenderBuffer
	{
		float damageHighlight = 0;
		DamageHighlightData damageHighlightData{};
	};

public:
	PlayerRenderBufferComponent(UID id, GameObject* owner);
	virtual std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void drawUi() override;
	void onTransformChange() override {}

	void serialize(IArchive& archive) override;

	void debugDraw() override {}

	void setDamageHighlightIntensity(float value) { m_damageHighlight = value; }
	float getDamageHighlightIntensity() { return m_damageHighlight; }
	void setDamageHighlightCenterColor(Vector3 value) { m_damageHighlightData.centerColor = value; }
	Vector3 getDamageHighlightCenterColor() { return m_damageHighlightData.centerColor; }
	void setDamageHighlightRimColor(Vector3 value) { m_damageHighlightData.rimColor = value; }
	Vector3 getDamageHighlightRimColor() { return m_damageHighlightData.rimColor; }
	void setDamageHighlightRimIntensity(float value) { m_damageHighlightData.rimIntensity = value; }
	float getDamageHighlightRimIntensity() { return m_damageHighlightData.rimIntensity; }

	DamageHighlightData getDamageHighlightData() { return m_damageHighlightData; }

private:
	float m_damageHighlight = 0;
	DamageHighlightData m_damageHighlightData{};
};

