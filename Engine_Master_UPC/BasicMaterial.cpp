#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleAssets.h"

#include "Texture.h"
#include "ModelAsset.h"

BasicMaterial::BasicMaterial(const UID uid, const MaterialAsset& asset)
	: ICacheable(uid)
{
	//Creates a handle to store all textures contigously instead of having a handler for each texture
	m_texturesHandle = app->getDescriptorsModule()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).allocate();

	UINT descriptorSize = app->getD3D12Module()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	if (asset.getBaseMap() != INVALID_ASSET_ID)
	{
		TextureAsset* baseMapTexture = static_cast<TextureAsset*>(app->getAssetModule()->requestAsset(asset.getBaseMap()));
		m_textureColor = app->getResourcesModule()->createTexture2D(*baseMapTexture, &m_texturesHandle);
		m_materialData.hasDiffuseTex = true;
	}
	else
	{
		m_textureColor = app->getResourcesModule()->createNullTexture2D(&m_texturesHandle);
		m_materialData.hasDiffuseTex = false;
	}

	m_texturesHandle.cpu.ptr += descriptorSize;
	m_texturesHandle.gpu.ptr += descriptorSize;
	m_texturesHandle.handle += 1;

	if (asset.getMetallicRoughnessMap() != INVALID_ASSET_ID)
	{
		TextureAsset* metallicRoughnessTexture = static_cast<TextureAsset*>(app->getAssetModule()->requestAsset(asset.getMetallicRoughnessMap()));
		m_textureMetallicRoughness = app->getResourcesModule()->createTexture2D(*metallicRoughnessTexture, & m_texturesHandle);
		m_materialData.hasMetallicRoughnessTex = true;
	}
	else
	{
		m_textureMetallicRoughness = app->getResourcesModule()->createNullTexture2D(&m_texturesHandle);
		m_materialData.hasMetallicRoughnessTex = false;
	}

	m_materialData.diffuseColour = Vector3(asset.getBaseColour().R(), asset.getBaseColour().G(), asset.getBaseColour().B());
	m_materialData.metallicFactor = asset.getMetallicFactor();
	//m_materialData.roughnessFactor = asset.getRoughnessFactor(); //Missing roughness factor in MaterialAsset class
	m_materialBuffer = app->getResourcesModule()->createDefaultBuffer(&m_materialData, alignUp(sizeof(PbrMetallicRoughnessData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), "MaterialBuffer");
}

BasicMaterial::~BasicMaterial()
{
	app->getModuleResources()->defferResourceRelease(m_materialBuffer);
}

Texture* BasicMaterial::getTexture() const noexcept
{
	return m_textureColor.get();
}
