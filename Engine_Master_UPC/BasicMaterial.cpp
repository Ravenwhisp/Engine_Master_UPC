#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleAssets.h"
#include "MaterialAsset.h"
#include "TextureAsset.h"
#include "Texture.h"

BasicMaterial::BasicMaterial(const UID uid, const MaterialAsset& asset) : ICacheable(uid)
{
	m_descriptors.resize(m_numAllocatedDescriptors);
	for (DescriptorHandle handle : m_descriptors)
	{
		handle = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).allocate();
	}

	if (asset.getBaseMap() != INVALID_ASSET_ID)
	{
		auto baseMapTexture = app->getModuleAssets()->load<TextureAsset>(asset.getBaseMap());
		m_textureColor = app->getModuleResources()->createTexture(*baseMapTexture);
		m_materialData.hasDiffuseTex = true;
	}
	else
	{
		m_textureColor.reset(app->getModuleResources()->createNullTexture2D());
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
	app->getModuleResources()->deferResourceRelease(m_materialBuffer);

}

Texture* BasicMaterial::getTexture() const noexcept
{
	return m_textureColor.get();
}
