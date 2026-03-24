#pragma once
#include "MD5Fwd.h"

struct SkyBoxSettings
{
	bool enabled = true;
	MD5Hash cubemapAssetId = INVALID_ASSET_ID;

	bool operator==(const SkyBoxSettings& o) const
    {
        return enabled == o.enabled && cubemapAssetId == o.cubemapAssetId;
    }
};