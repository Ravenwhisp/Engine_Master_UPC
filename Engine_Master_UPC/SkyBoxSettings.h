#pragma once
#include "MD5Fwd.h"

struct SkyBoxSettings
{
	bool enabled = true;
	MD5Hash cubemapAssetId = INVALID_ASSET_ID;
};