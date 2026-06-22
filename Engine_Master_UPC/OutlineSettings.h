#pragma once
#include "ISerializable.h"
#include "IArchive.h"
#include "SimpleMath.h"

struct OutlineSettings : public ISerializable
{
	bool enabled = false;
	DirectX::SimpleMath::Color outlineColor = DirectX::SimpleMath::Color(1.0f, 1.0f, 0.0f, 1.0f);
	float outlineThickness = 2.0f;

	bool operator==(const OutlineSettings& o) const
	{
		return enabled == o.enabled
			&& outlineColor == o.outlineColor
			&& outlineThickness == o.outlineThickness;
	}

	void serialize(IArchive& archive) override
	{
		archive.serialize(enabled, "enabled");
		archive.serialize(outlineColor, "outlineColor");
		archive.serialize(outlineThickness, "outlineThickness");
	}
};
