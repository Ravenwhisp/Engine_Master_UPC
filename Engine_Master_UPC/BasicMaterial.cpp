#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleAssets.h"
#include "MaterialAsset.h"
#include "TextureAsset.h"
#include "Texture.h"

BasicMaterial::BasicMaterial(const UID uid, const MaterialAsset& asset)
	: ICacheable(uid)
{
	if (asset.getBaseMap() != INVALID_ASSET_ID)
	{
		auto baseMapTexture = app->getModuleAssets()->load<TextureAsset>(asset.getBaseMap());
		m_textureColor = app->getModuleResources()->createTextureLinear(*baseMapTexture);
		m_materialData.hasDiffuseTex = true;
	}
	else
	{
		m_textureColor.reset(app->getModuleResources()->createNullTexture2D());
		m_materialData.hasDiffuseTex = false;
	}

	m_materialData.diffuseColour = Vector3(asset.getBaseColour().R(), asset.getBaseColour().G(), asset.getBaseColour().B());
	m_materialData.specularColour = Vector3(0.1f, 0.1f, 0.1f);
	m_materialData.shininess = 32.0f;
	m_materialBuffer = app->getModuleResources()->createDefaultBuffer(&m_materialData, alignUp(sizeof(BDRFPhongMaterialData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), "MaterialBuffer");
}

BasicMaterial::~BasicMaterial()
{
	app->getModuleResources()->deferResourceRelease(m_materialBuffer);

}

Texture* BasicMaterial::getTexture() const noexcept
{
	return m_textureColor.get();
}
