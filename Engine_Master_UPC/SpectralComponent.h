#pragma once
#include "Component.h"



struct SpectralData 
{
	Vector3 spectralColor;
};

struct SpectralCB
{
	UINT hasSpectralComponent;
	Vector3 padding1 = Vector3::Zero;

	SpectralData spectralData;
	UINT padding2;
};



class SpectralComponent : public Component
{
public:
	SpectralComponent(UID id, GameObject* owner);
	virtual std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void drawUi() override;
	void onTransformChange() override {}

	void serialize(IArchive& archive) override;

	void debugDraw() override {}

	void setSpectralColor(Vector3 value) { m_spectralData.spectralColor = value; }
	Vector3 getSpectralColor() { return m_spectralData.spectralColor; }

	SpectralData getSpectralData() { return m_spectralData; }

private:
	SpectralData m_spectralData{};
};

