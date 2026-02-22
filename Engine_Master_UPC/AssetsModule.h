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

	int find(const std::filesystem::path& assetsFile) const;
	int import(const std::filesystem::path & assetsFile);
	int generateNewUID();

	Asset*	requestAsset(int id);
	void	releaseAsset(Asset* asset);

private:
	std::unordered_map<int, Asset*> m_assets;
};