#pragma once
#include "Module.h"
#include <Asset.h>

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