#pragma once

#include "ICacheable.h"

#include <memory>
#include <wrl/client.h>
#include <d3d12.h>

#include "SimpleMath.h"

class Texture;
class MaterialAsset;

using Microsoft::WRL::ComPtr;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;

class BasicMaterial : public ICacheable
{
public:
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

	explicit BasicMaterial(const MD5Hash uid, const MaterialAsset& asset);
	~BasicMaterial();

	ComPtr<ID3D12Resource>		getMaterialBuffer() const noexcept { return m_materialBuffer; }
	Texture*					getTexture() const noexcept;
	BDRFPhongMaterialData&		getMaterial() { return m_materialData; }

	void setMaterial(BDRFPhongMaterialData& material) { m_materialData = material; }
private:
	std::shared_ptr<Texture>	m_textureColor;
	ComPtr<ID3D12Resource>		m_materialBuffer;
	BDRFPhongMaterialData		m_materialData;
};
