#pragma once
#include "SimpleMath.h"
#include "ISerializable.h"
#include "IArchive.h"

struct SceneLightingSettings : public ISerializable
{
	DirectX::SimpleMath::Vector3 ambientColor;
	float ambientIntensity;

	bool tileDebugView = false;

	void serialize(IArchive& archive) override
	{
		archive.serialize(ambientColor, "ambientColor");
		archive.serialize(ambientIntensity, "ambientIntensity");
		archive.serialize(tileDebugView, "tileDebugView");
	}
};
