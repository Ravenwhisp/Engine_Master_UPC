#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleDescriptors.h"
#include "ModuleD3D12.h"
#include "ModuleAssets.h"
#include "MaterialAsset.h"
#include "TextureAsset.h"
#include "Texture.h"

BasicMaterial::BasicMaterial(const UID uid, MaterialAsset& asset) : ICacheable(uid)
{

	if (asset.getBaseMap().isValid())
	{
		auto baseMapTexture = app->getModuleAssets()->load<TextureAsset>(asset.getBaseMap());
		m_textureColor = app->getModuleResources()->createTexture(*baseMapTexture);
		m_materialData.hasDiffuseTex = true;
	}
	else
	{
		m_textureColor = app->getModuleResources()->createNullTexture2D();
		m_materialData.hasDiffuseTex = false;
	}

	if (asset.getMetallicRoughnessMap().isValid())
	{
		auto metallicRoughnessTexture = app->getModuleAssets()->load<TextureAsset>(asset.getMetallicRoughnessMap());

		m_textureMetallicRoughness = app->getModuleResources()->createTexture(*metallicRoughnessTexture);
		m_materialData.hasMetallicRoughnessTex = true;
	}
	else
	{
		m_textureMetallicRoughness = app->getModuleResources()->createNullTexture2D();
		m_materialData.hasMetallicRoughnessTex = false;
	}

	if (asset.getNormalMap().isValid())
	{
		auto normalTexture = app->getModuleAssets()->load<TextureAsset>(asset.getNormalMap());

		m_textureNormal = app->getModuleResources()->createTexture(*normalTexture);
		m_materialData.hasNormalTex = true;
	}
	else
	{
		m_textureNormal = app->getModuleResources()->createNullTexture2D();
		m_materialData.hasNormalTex = false;
	}

	if (asset.getEmissive().isValid())
	{
		auto emissiveTexture = app->getModuleAssets()->load<TextureAsset>(asset.getEmissive());
		m_textureEmissive = app->getModuleResources()->createTexture(*emissiveTexture);
		m_materialData.hasEmissiveTex = true;
	}
	else
	{
		m_textureEmissive = app->getModuleResources()->createNullTexture2D();
		m_materialData.hasEmissiveTex = false;
	}

	m_materialData.diffuseColour = Vector3(asset.getBaseColour().R(), asset.getBaseColour().G(), asset.getBaseColour().B());
	m_materialData.metallicFactor = asset.getMetallicFactor();
	m_materialData.roughnessFactor = asset.getRoughnessFactor();
	m_materialData.normalFactor = asset.getNormalFactor();
	m_materialData.emmisiveColour = Vector3(1, 1, 1);
	m_materialBuffer = app->getModuleResources()->createDefaultBuffer(&m_materialData, alignUp(sizeof(PbrMetallicRoughnessData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), "MaterialBuffer");

    buildDescriptorTable();
}

BasicMaterial::~BasicMaterial()
{
	app->getModuleResources()->deferResourceRelease(m_materialBuffer);

}

Texture* BasicMaterial::getTexture() const noexcept
{
	return m_textureColor.get();
}

D3D12_GPU_DESCRIPTOR_HANDLE BasicMaterial::getTableGPUHandle() const
{
    assert(m_block && "Material descriptor table was not built");
    return m_block->getGPUHandle(0);
}

void BasicMaterial::buildDescriptorTable()
{
    DescriptorHeap& srvHeap =  app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Allocate one contiguous block covering all material texture slots.
    m_block = srvHeap.allocateBlock(SLOT_COUNT);
    assert(m_block && "Failed to allocate material descriptor block");

    for (uint32_t slot = 0; slot < SLOT_COUNT; ++slot)
    {
        writeNullDescriptor(slot);
    }

    // Copy each texture's SRV into the corresponding slot of our block,
    // then release the texture's own private 1-slot block.
    copyTextureIntoSlot(m_textureColor.get(), SLOT_DIFFUSE);
    copyTextureIntoSlot(m_textureMetallicRoughness.get(), SLOT_METAL);
	copyTextureIntoSlot(m_textureNormal.get(), SLOT_NORMAL);
	copyTextureIntoSlot(m_textureEmissive.get(), SLOT_EMISSIVE);

}

void BasicMaterial::copyTextureIntoSlot(Texture* texture, uint32_t slot)
{
    assert(texture && "Cannot copy a null texture into the descriptor table");
    assert(slot < SLOT_COUNT);

    ID3D12Device* device = app->getModuleD3D12()->getDevice();
    DescriptorHeap& srvHeap = app->getModuleDescriptors()->getHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Copy the SRV descriptor from the texture's private block into our slot.
    device->CopyDescriptorsSimple( 1,
        m_block->getCPUHandle(slot),   // dst: our material table slot
        texture->getSRV().cpu,                  // src: texture's own descriptor
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}

void BasicMaterial::writeNullDescriptor(uint32_t slot)
{
    assert(slot < SLOT_COUNT);

    // Write a null SRV so unbound slots return 0 when sampled in the shader,
    // instead of reading undefined heap memory.
    D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc{};
    nullDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    nullDesc.Texture2D.MipLevels = 1;

    app->getModuleD3D12()->getDevice()->CreateShaderResourceView(
        nullptr,                        // null resource → null descriptor
        &nullDesc,
        m_block->getCPUHandle(slot));
}