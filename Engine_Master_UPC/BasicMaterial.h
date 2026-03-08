#pragma once
#include "Globals.h"
#include "Texture.h"
#include "Asset.h"
#include "ModelAsset.h"
#include "ICacheable.h"

namespace tinygltf { class Model; struct Material; struct PbrMetallicRoughness; }
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

	struct PbrMetallicRoughnessData
	{
		Vector3     diffuseColour;
		BOOL        hasDiffuseTex;

		BOOL        hasMetallicRoughnessTex;
		float       metallicFactor;
		float       roughnessFactor;
		float       padding;
	};

	explicit BasicMaterial(const UID uid, const MaterialAsset& asset);
	~BasicMaterial();

	ComPtr<ID3D12Resource>		getMaterialBuffer() const { return m_materialBuffer; }
	Texture*					getTexture() const { return m_textureColor.get(); }
	PbrMetallicRoughnessData&	getMaterial() { return m_materialData; }
	DescriptorHandle			getTexturesHandle() const { return m_texturesHandle; }

	void setMaterial(PbrMetallicRoughnessData& material) { m_materialData = material; }

private:
	std::shared_ptr<Texture>	m_textureColor;
	std::shared_ptr<Texture>	m_textureMetallicRoughness;
	ComPtr<ID3D12Resource>		m_materialBuffer;
	PbrMetallicRoughnessData	m_materialData;
	DescriptorHandle			m_texturesHandle;
};
	



