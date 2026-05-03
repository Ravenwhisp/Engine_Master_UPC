#pragma once
#include "AssetReference.h"

struct SkyBoxSettings
{
	bool enabled = true;
	AssetReference cubemapAssetId{};

	bool operator==(const SkyBoxSettings& o) const
    {
        return enabled == o.enabled && cubemapAssetId == o.cubemapAssetId;
    }
};