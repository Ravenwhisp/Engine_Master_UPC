#pragma once
#include "Module.h"
#include <Asset.h>

constexpr const char* ASSETS_FOLDER = "Assets/";
constexpr const char* LIBRARY_FOLDER = "Library/";
constexpr const char* ASSET_EXTENSION = ".asset";
constexpr const char* METADATA_EXTENSION = ".metadata";

class AssetsModule : public Module
{
public:

	UID find(const std::filesystem::path& assetsFile) const;
	UID import(const std::filesystem::path & assetsFile);

	Asset*	requestAsset(UID id);
	Asset*  requestAsset(const AssetMetadata* metadata);
	void	releaseAsset(Asset* asset);

private:
	std::unordered_map<UID, Asset*> m_assets;
};