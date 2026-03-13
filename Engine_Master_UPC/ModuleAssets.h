#pragma once
#include "Module.h"

#include "UID.h"
#include "AssetsDictionary.h"

#include <filesystem>
#include <unordered_map>

class Asset;
class AssetMetadata;

class ModuleAssets : public Module
{
public:

	UID importAsset(const std::filesystem::path & assetsFile, UID uid = INVALID_ASSET_ID);

	Asset*	requestAsset(UID id);
	Asset*  requestAsset(const AssetMetadata* metadata);
	void	releaseAsset(Asset* asset);

private:
	std::unordered_map<UID, Asset*> m_assets;
};