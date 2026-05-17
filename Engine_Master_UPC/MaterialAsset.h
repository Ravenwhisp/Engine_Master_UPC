#pragma once
#include "Globals.h"

#include "Asset.h"

class MaterialAsset : public Asset
{
public:
	friend class ModelImporter;
	friend class ImporterMaterial;
	friend class ImporterGltf;

	MaterialAsset() = default;
	MaterialAsset(AssetReference& id) : Asset(id, AssetType::MATERIAL) {}

	AssetReference& getBaseMap() { return baseMap; }
	Color& getBaseColour() const { return baseColour; }

	AssetReference& getMetallicRoughnessMap() { return metallicRoughnessMap; }
	AssetReference& getNormalMap() { return normalMap; }
	uint32_t getMetallicFactor() const { return metallicFactor; }
	uint32_t getRoughnessFactor() const { return roughnessFactor; }
	uint32_t getNormalFactor() const { return normalFactor; }
	AssetReference& getEmissive() { return emissiveMap; }
protected:

	AssetReference		baseMap{};
	mutable Color		baseColour = Color(255, 255, 255, 0);

	AssetReference		metallicRoughnessMap{};
	uint32_t			roughnessFactor = 0;
	uint32_t			metallicFactor = 0;
	AssetReference		normalMap{};
	uint32_t			normalFactor = 0;
	AssetReference		occlusionMap{};

	bool				isEmissive = false;
	AssetReference		emissiveMap{};
};

