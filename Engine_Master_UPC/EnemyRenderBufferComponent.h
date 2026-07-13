#pragma once
#include "Component.h"
class EnemyRenderBufferComponent : public Component
{
public:
	struct DamageHighlightData
	{
		Vector3 centerColor = Vector3::One;
		Vector3 rimColor = Vector3::One;
		float rimIntensity = 1;
	};

	struct DissolveEffectData
	{
		Vector3 rimColor = Vector3::One;
		float rimIntensity = 1;
	};

	struct EnemyRenderBuffer
	{
		float damageHighlight = 0;
		DamageHighlightData damageHighlightData{};

		float dissolveEffect = 0;
		DissolveEffectData dissolveEffectData{};

		Vector3 align = Vector3::Zero;
	};

public:
	EnemyRenderBufferComponent(UID id, GameObject* owner);
	virtual std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void drawUi() override;
	void onTransformChange() override {}

	void serialize(IArchive& archive) override;

	void debugDraw() override {}

	void    setDamageHighlightIntensity(float value)     { m_damageHighlight = value; }
	float   getDamageHighlightIntensity()                { return m_damageHighlight; }
	void    setDamageHighlightCenterColor(Vector3 value) { m_damageHighlightData.centerColor = value; }
	Vector3 getDamageHighlightCenterColor()              { return m_damageHighlightData.centerColor; }
	void    setDamageHighlightRimColor(Vector3 value)    { m_damageHighlightData.rimColor = value; }
	Vector3 getDamageHighlightRimColor()                 { return m_damageHighlightData.rimColor; }
	void    setDamageHighlightRimIntensity(float value)  { m_damageHighlightData.rimIntensity = value; }
	float   getDamageHighlightRimIntensity()             { return m_damageHighlightData.rimIntensity; }

	void    setDissolveEffectIntensity(float value)    { m_dissolveEffect = value; }
	float   getDissolveEffectIntensity()               { return m_dissolveEffect; }
	void    setDissolveEffectRimColor(Vector3 value)   { m_dissolveEffectData.rimColor = value; }
	Vector3 getDissolveEffectRimColor()                { return m_dissolveEffectData.rimColor; }
	void    setDissolveEffectRimIntensity(float value) { m_dissolveEffectData.rimIntensity = value; }
	float   getDissolveEffectRimIntensity()            { return m_dissolveEffectData.rimIntensity; }

	DamageHighlightData getDamageHighlightData() { return m_damageHighlightData; }
	DissolveEffectData  getDissolveEffectData()  { return m_dissolveEffectData; }

private:
	float m_damageHighlight = 0;
	DamageHighlightData m_damageHighlightData{};

	float m_dissolveEffect = 0;
	DissolveEffectData m_dissolveEffectData{};
};

