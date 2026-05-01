#pragma once
#include "UID.h"

struct SkyBoxSettings
{
	bool enabled = true;
	UID cubemapAssetId = INVALID_UID;

	bool operator==(const SkyBoxSettings& o) const
    {
        return enabled == o.enabled && cubemapAssetId == o.cubemapAssetId;
    }
};