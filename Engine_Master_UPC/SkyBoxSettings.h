#pragma once
#include "MD5Fwd.h"

struct SkyBoxSettings
{
	bool enabled = true;
	MD5Hash cubemapAssetId = "51513e627d3296012beac59084735eaa";

	bool operator==(const SkyBoxSettings& o) const
    {
        return enabled == o.enabled && cubemapAssetId == o.cubemapAssetId;
    }
};