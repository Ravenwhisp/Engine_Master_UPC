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
	MaterialAsset(UID id) : Asset(id, AssetType::MATERIAL) {}

	UID getBaseMap() const { return baseMap; }
	Color& getBaseColour() const { return baseColour; }

	UID getMetallicRoughnessMap() const { return metallicRoughnessMap; }
	uint32_t getMetallicFactor() const { return metallicFactor; }
	uint32_t getRoughnessFactor() const { return roughnessFactor; }
protected:

	UID				baseMap = INVALID_UID;
	mutable Color		baseColour = Color(255, 255, 255, 0);

	UID				metallicRoughnessMap = INVALID_UID;
	uint32_t			roughnessFactor = 0;
	uint32_t			metallicFactor = 0;
	UID				normalMap = INVALID_UID;
	UID				occlusionMap = INVALID_UID;

	bool				isEmissive = false;
	UID 			emissiveMap = INVALID_UID;
};

