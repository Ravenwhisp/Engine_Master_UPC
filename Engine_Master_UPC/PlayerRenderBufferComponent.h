#pragma once
#include "Component.h"

class PlayerRenderBufferComponent : public Component
{
public:
	struct PlayerRenderBuffer
	{
		float damageHighlight;

		Vector3 align;
	};

public:
	PlayerRenderBufferComponent(UID id, GameObject* owner);
	virtual std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void drawUi() override;
	void onTransformChange() override {}

	void serialize(IArchive& archive) override;

	void debugDraw() override {}

	void setDamageHighlight(float value);
	float getDamageHighlight();

private:
	float m_damageHighlight = 0;
};

