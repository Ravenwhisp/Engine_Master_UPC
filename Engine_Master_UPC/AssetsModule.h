#pragma once
#include "Module.h"
#include "Asset.h"
#include "WeakCache.h"
#include "Delegates.h"

class ImportRequest;

class AssetsModule : public Module
{
public:
	bool init()    override;
	bool cleanUp() override;

	UID import(const std::filesystem::path & assetsFile, UID uid = INVALID_ASSET_ID);

	std::shared_ptr<Asset>	requestAsset(UID id);
	std::shared_ptr<Asset> loadAsset(const AssetMetadata* metadata);

private:
	void onImportRequested(const ImportRequest& request);

	WeakCache<UID, Asset>  m_assets;
	DelegateHandle    m_importHandle;
};