#pragma once
#include "Globals.h"
#include "Texture.h"
#include "Asset.h"
#include "ModelAsset.h"

namespace tinygltf { class Model; struct Material; struct PbrMetallicRoughness; }
class BasicMaterial
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

	explicit BasicMaterial(const MaterialAsset& asset);
	~BasicMaterial() = default;

	ComPtr<ID3D12Resource>		getMaterialBuffer() const { return m_materialBuffer; }
	Texture*					getTexture() const { return m_textureColor.get(); }
	BDRFPhongMaterialData&		getMaterial() { return m_materialData; }

	void setMaterial(BDRFPhongMaterialData& material) { m_materialData = material; }
private:
	std::unique_ptr<Texture>	m_textureColor;
	ComPtr<ID3D12Resource>		m_materialBuffer;
	BDRFPhongMaterialData		m_materialData;
};
	



