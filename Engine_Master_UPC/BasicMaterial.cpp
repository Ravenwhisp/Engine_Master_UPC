#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleAssets.h"

#include "Texture.h"
#include "ModelAsset.h"

BasicMaterial::BasicMaterial(const UID uid, const MaterialAsset& asset) : ICacheable(uid)
{
	m_descriptors.resize(m_numAllocatedDescriptors);
	for (DescriptorHandle handle : m_descriptors)
	{
		handle = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).allocate();
	}

	if (asset.getBaseMap() != INVALID_ASSET_ID)
	{
		TextureAsset* baseMapTexture = static_cast<TextureAsset*>(app->getAssetModule()->requestAsset(asset.getBaseMap()));
		m_textureColor = app->getModuleResources()->createTexture2DWithDescriptor(*baseMapTexture, &m_descriptors[0]);
		m_materialData.hasDiffuseTex = true;
	}
	else
	{
		m_textureColor = app->getModuleResources()->createNullTexture2D();
		m_materialData.hasDiffuseTex = false;
	}

	if (asset.getMetallicRoughnessMap() != INVALID_ASSET_ID)
	{
		TextureAsset* metallicRoughnessTexture = static_cast<TextureAsset*>(app->getAssetModule()->requestAsset(asset.getMetallicRoughnessMap()));
		m_textureMetallicRoughness = app->getModuleResources()->createTexture2DWithDescriptor(*metallicRoughnessTexture, &m_descriptors[1]);
		m_materialData.hasMetallicRoughnessTex = true;
	}
	else
	{
		m_textureMetallicRoughness = app->getModuleResources()->createNullTexture2D();
		m_materialData.hasMetallicRoughnessTex = false;
	}

	m_materialData.diffuseColour = Vector3(asset.getBaseColour().R(), asset.getBaseColour().G(), asset.getBaseColour().B());
	m_materialData.metallicFactor = asset.getMetallicFactor();
	//m_materialData.roughnessFactor = asset.getRoughnessFactor(); //Missing roughness factor in MaterialAsset class
	m_materialBuffer = app->getModuleResources()->createDefaultBuffer(&m_materialData, alignUp(sizeof(PbrMetallicRoughnessData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), "MaterialBuffer");
}

BasicMaterial::~BasicMaterial()
{
	app->getModuleResources()->defferResourceRelease(m_materialBuffer);
}

Texture* BasicMaterial::getTexture() const noexcept
{
	return m_textureColor.get();
}
