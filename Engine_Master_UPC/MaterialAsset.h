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

	AssetReference& getBaseMap() { return baseMap; }
	Color& getBaseColour() const { return baseColour; }

	AssetReference& getMetallicRoughnessMap() { return metallicRoughnessMap; }
	uint32_t getMetallicFactor() const { return metallicFactor; }
	uint32_t getRoughnessFactor() const { return roughnessFactor; }

#pragma region Serialization
	template <class Archive>

	void serialize(Archive& ar)
	{
		ar(cereal::base_class<Asset>(this), baseMap, baseColour, metallicRoughnessMap, roughnessFactor, metallicFactor, normalMap, occlusionMap, isEmissive, emissiveMap);
	}
#pragma endregion
protected:

	AssetReference		baseMap;
	mutable Color		baseColour = Color(255, 255, 255, 0);

	AssetReference		metallicRoughnessMap;
	uint32_t			roughnessFactor = 0;
	uint32_t			metallicFactor = 0;
	AssetReference		normalMap;
	AssetReference		occlusionMap;

	bool				isEmissive = false;
	AssetReference 		emissiveMap;
};

CEREAL_REGISTER_TYPE(MaterialAsset)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Asset, MaterialAsset)
