#pragma once
#include "Globals.h"

#include "Asset.h"
#include "IArchive.h"

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

	void serialize(IArchive& archive) override
	{
		archive.serialize(baseMap.m_uid);
		archive.serialize(baseMap.m_libId);
		uint32_t baseType = static_cast<uint32_t>(baseMap.m_type);
		archive.serialize(baseType);
		baseMap.m_type = static_cast<AssetType>(baseType);

		archive.serializeRaw(&baseColour, sizeof(Color));

		archive.serialize(metallicRoughnessMap.m_uid);
		archive.serialize(metallicRoughnessMap.m_libId);
		uint32_t mrmType = static_cast<uint32_t>(metallicRoughnessMap.m_type);
		archive.serialize(mrmType);
		metallicRoughnessMap.m_type = static_cast<AssetType>(mrmType);

		archive.serialize(metallicFactor);
		archive.serialize(roughnessFactor);

		archive.serialize(normalMap.m_uid);
		archive.serialize(normalMap.m_libId);
		uint32_t nmType = static_cast<uint32_t>(normalMap.m_type);
		archive.serialize(nmType);
		normalMap.m_type = static_cast<AssetType>(nmType);

		archive.serialize(occlusionMap.m_uid);
		archive.serialize(occlusionMap.m_libId);
		uint32_t omType = static_cast<uint32_t>(occlusionMap.m_type);
		archive.serialize(omType);
		occlusionMap.m_type = static_cast<AssetType>(omType);

		archive.serialize(isEmissive);

		archive.serialize(emissiveMap.m_uid);
		archive.serialize(emissiveMap.m_libId);
		uint32_t emType = static_cast<uint32_t>(emissiveMap.m_type);
		archive.serialize(emType);
		emissiveMap.m_type = static_cast<AssetType>(emType);
	}

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

