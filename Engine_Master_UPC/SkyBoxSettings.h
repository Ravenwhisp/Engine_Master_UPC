#pragma once
#include "AssetReference.h"
#include "ISerializable.h"
#include "IArchive.h"

struct SkyBoxSettings : public ISerializable
{
	bool enabled = true;
	AssetReference cubemapAssetId{};

	bool operator==(const SkyBoxSettings& o) const
    {
        return enabled == o.enabled && cubemapAssetId == o.cubemapAssetId;
    }

	void serialize(IArchive& archive) override
	{
		archive.serialize(enabled, "enabled");
		cubemapAssetId.serialize(archive);
	}
};
