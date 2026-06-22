#pragma once
#include "ISerializable.h"
#include "IArchive.h"
#include "SimpleMath.h"
#include "AssetReference.h"

struct OutlineSettings : public ISerializable
{
	bool enabled = false;
	DirectX::SimpleMath::Color colorModifier = DirectX::SimpleMath::Color(0.3f, 0.05f, 0.1f, 1.0f);
	float minSeparation = 1.0f;
	float maxSeparation = 3.0f;
	float minDistance = 0.01f;
	float maxDistance = 0.50f;
	int   searchSize = 1;
	float noiseScale = 0.0f;
	AssetReference noiseTextureAssetId{};

	bool operator==(const OutlineSettings& o) const
	{
		return enabled == o.enabled
			&& colorModifier == o.colorModifier
			&& minSeparation == o.minSeparation
			&& maxSeparation == o.maxSeparation
			&& minDistance == o.minDistance
			&& maxDistance == o.maxDistance
			&& searchSize == o.searchSize
			&& noiseScale == o.noiseScale
			&& noiseTextureAssetId == o.noiseTextureAssetId;
	}

	void serialize(IArchive& archive) override
	{
		archive.serialize(enabled, "enabled");
		archive.serialize(colorModifier, "colorModifier");
		archive.serialize(minSeparation, "minSeparation");
		archive.serialize(maxSeparation, "maxSeparation");
		archive.serialize(minDistance, "minDistance");
		archive.serialize(maxDistance, "maxDistance");
		uint32_t s = static_cast<uint32_t>(searchSize);
		archive.serialize(s, "searchSize");
		if (archive.mode() == ArchiveMode::Input)
			searchSize = static_cast<int>(s);
		archive.serialize(noiseScale, "noiseScale");
		noiseTextureAssetId.serialize(archive);
	}
};
