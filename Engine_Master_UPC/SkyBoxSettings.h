#pragma once
#include "UID.h"
#include "AssetReference.h"

struct SkyBoxSettings
{
    friend class cereal::access;

	bool enabled = true;
	AssetReference cubemapAssetId;

	bool operator==(const SkyBoxSettings& o) const
    {
        return enabled == o.enabled && cubemapAssetId == o.cubemapAssetId;
    }

#pragma once Serialization
    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(enabled, cubemapAssetId);
    }
#pragma endregion
};