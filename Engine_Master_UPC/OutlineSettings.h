#pragma once
#include "ISerializable.h"
#include "IArchive.h"
#include "SimpleMath.h"

struct OutlineSettings : public ISerializable
{
	bool enabled = false;
	DirectX::SimpleMath::Color outlineColor = DirectX::SimpleMath::Color(1.0f, 1.0f, 0.0f, 1.0f);
	float minSeparation = 1.0f;
	float maxSeparation = 3.0f;
	float minDistance = 0.001f;
	float maxDistance = 0.03f;
	int   searchSize = 1;
	float noiseScale = 0.0f;

	bool operator==(const OutlineSettings& o) const
	{
		return enabled == o.enabled
			&& outlineColor == o.outlineColor
			&& minSeparation == o.minSeparation
			&& maxSeparation == o.maxSeparation
			&& minDistance == o.minDistance
			&& maxDistance == o.maxDistance
			&& searchSize == o.searchSize
			&& noiseScale == o.noiseScale;
	}

	void serialize(IArchive& archive) override
	{
		archive.serialize(enabled, "enabled");
		archive.serialize(outlineColor, "outlineColor");
		archive.serialize(minSeparation, "minSeparation");
		archive.serialize(maxSeparation, "maxSeparation");
		archive.serialize(minDistance, "minDistance");
		archive.serialize(maxDistance, "maxDistance");
		uint32_t s = static_cast<uint32_t>(searchSize);
		archive.serialize(s, "searchSize");
		if (archive.mode() == ArchiveMode::Input)
			searchSize = static_cast<int>(s);
		archive.serialize(noiseScale, "noiseScale");
	}
};
