#pragma once
#include "Module.h"
#include <Asset.h>



class AssetsModule : public Module
{
public:

	UID find(const std::filesystem::path& assetsFile) const;
	UID import(const std::filesystem::path & assetsFile, UID uid = INVALID_ASSET_ID);

	Asset*	requestAsset(UID id);
	Asset*  requestAsset(const AssetMetadata* metadata);
	void	releaseAsset(Asset* asset);

private:
	std::unordered_map<UID, Asset*> m_assets;
};