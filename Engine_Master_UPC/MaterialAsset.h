#pragma once
#include "Globals.h"

#include "Asset.h"

class MaterialAsset : public Asset
{
public:
	friend class ModelImporter;
	friend class ImporterMaterial;
	friend class ImporterGltf;

	MaterialAsset() {}
	MaterialAsset(MD5Hash id) : Asset(id, AssetType::MATERIAL) {}

	MD5Hash getBaseMap() const { return baseMap; }
	Color& getBaseColour() const { return baseColour; }
protected:

	MD5Hash				baseMap = INVALID_ASSET_ID;
	mutable Color		baseColour = Color(255, 255, 255, 0);

	MD5Hash				metallicRoughnessMap = INVALID_ASSET_ID;
	uint32_t			metallicFactor = 0;
	MD5Hash				normalMap = INVALID_ASSET_ID;
	MD5Hash				occlusionMap = INVALID_ASSET_ID;

	bool				isEmissive = false;
	MD5Hash 			emissiveMap = INVALID_ASSET_ID;
};

