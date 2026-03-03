#pragma once
#include "Module.h"
#include <Asset.h>



class AssetsModule : public Module
{
public:

	UID find(const std::filesystem::path& assetsFile) const;
	UID import(const std::filesystem::path & assetsFile, UID uid = INVALID_ASSET_ID);

	std::shared_ptr<Asset>	requestAsset(UID id);
	std::shared_ptr<Asset>  requestAsset(const AssetMetadata* metadata);

private:
	std::unordered_map<UID, std::shared_ptr<Asset>> m_assets;
};