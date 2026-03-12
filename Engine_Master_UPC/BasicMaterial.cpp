#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ResourcesModule.h"
#include "ModuleFlyweight.h"
#include "AssetsModule.h"
#include <TextureImporter.h>

BasicMaterial::BasicMaterial(const UID uid, const MaterialAsset& asset) : ICacheable(uid)
{
	if (asset.getBaseMap() != INVALID_ASSET_ID)
	{
		auto baseMapTexture = std::static_pointer_cast<TextureAsset>(app->getAssetModule()->requestAsset(asset.getBaseMap()));
		m_textureColor = app->getModuleFlyweight()->createTexture(*baseMapTexture);
		m_materialData.hasDiffuseTex = true;
	}
	else
	{
		m_textureColor.reset(app->getResourcesModule()->createNullTexture2D());
		m_materialData.hasDiffuseTex = false;
	}

	m_materialData.diffuseColour = Vector3(asset.getBaseColour().R(), asset.getBaseColour().G(), asset.getBaseColour().B());
	m_materialData.specularColour = Vector3(0.1f, 0.1f, 0.1f);
	m_materialData.shininess = 32.0f;
	m_materialBuffer = app->getResourcesModule()->createDefaultBuffer(&m_materialData, alignUp(sizeof(BDRFPhongMaterialData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), "MaterialBuffer");
}

BasicMaterial::~BasicMaterial()
{
	app->getResourcesModule()->deferResourceRelease(m_materialBuffer);
}
