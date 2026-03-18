#pragma once

#include "ICacheable.h"

#include <memory>
#include <wrl/client.h>
#include <d3d12.h>
#include "DescriptorHeapBlock.h"

#include "SimpleMath.h"

class Texture;
class MaterialAsset;

using Microsoft::WRL::ComPtr;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

class BasicMaterial : public ICacheable
{
public:

	enum TextureSlot : uint32_t
	{
		SLOT_DIFFUSE = 0,
		SLOT_METAL  = 1,
		// SLOT_ROUGHNESS = 2,
		SLOT_COUNT = 8
	};


	struct MaterialData {
		Vector4		baseColor;
		BOOL		hasColourTexture;  // use BOOL (4 bytes) instead of c++ bool (1 byte) as HLSL bool is 4 bytes long
	};

	struct PhongMaterialData
	{
		Vector4		diffuseColour;
		float		Kd;
		float		Ks;
		float		shininess;
		BOOL		hasDiffuseTex;
	};

	struct BDRFPhongMaterialData {
		Vector3		diffuseColour;
		BOOL		hasDiffuseTex;
		Vector3		specularColour;
		float		shininess;
	};

	struct PbrMetallicRoughnessData
	{
		Vector3     diffuseColour;
		BOOL        hasDiffuseTex;

		float       metallicFactor;
		float       roughnessFactor;
		BOOL        hasMetallicRoughnessTex;
		float       padding;
	};

	explicit BasicMaterial(const UID uid, const MaterialAsset& asset);
	~BasicMaterial();

	Texture* getTexture() const noexcept;

	ComPtr<ID3D12Resource>		getMaterialBuffer() const { return m_materialBuffer; }
	PbrMetallicRoughnessData&	getMaterial() { return m_materialData; }
	D3D12_GPU_DESCRIPTOR_HANDLE getTableGPUHandle() const;

	void setMaterial(PbrMetallicRoughnessData& material) { m_materialData = material; }

private:
	void buildDescriptorTable();
	void copyTextureIntoSlot(Texture* texture, uint32_t slot);
	void writeNullDescriptor(uint32_t slot);


	std::shared_ptr<Texture>	  m_textureColor;
	std::shared_ptr<Texture>	  m_textureMetallicRoughness;
	ComPtr<ID3D12Resource>		  m_materialBuffer;
	PbrMetallicRoughnessData	  m_materialData;

	DescriptorHeapBlock* m_block{};
};
