#pragma once
#include "UID.h"

struct SkyBoxSettings
{
	bool enabled = true;
	UID cubemapAssetId = 0;

	bool operator==(const SkyBoxSettings& o) const
    {
        return enabled == o.enabled && cubemapAssetId == o.cubemapAssetId;
    }
};