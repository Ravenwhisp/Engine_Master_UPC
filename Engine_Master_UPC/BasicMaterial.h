#pragma once
#include "Globals.h"
#include "Texture.h"
#include "Asset.h"

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

	//New
	struct PbrMetallicRoughnessData //QUESTION: Where do I get TextureInfo NO EXISTE
	{
		Vector3		diffuseColour;
		BOOL		hasDiffuseTex;
		BOOL		hasMetallicRoughnessTex;

		//Vector4 baseColorFactor;
		//TextureInfo baseColorTexture;
		//uint hasBaseColorTexture;
		//float metallicFactor;
		//float roughnessFactor;
		//TextureInfo metallicRoughnessTexture;*/
	};

	bool load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath);
	ComPtr<ID3D12Resource>	getMaterialBuffer() const { return m_materialBuffer; }
	Texture* getTexture() const { return m_textureColor.get(); }
	PbrMetallicRoughnessData& getMaterial() { return m_materialData; }
	void setMaterial(PbrMetallicRoughnessData& material) { m_materialData = material; }
	//BDRFPhongMaterialData& getMaterial() { return m_materialData; }
	//void setMaterial(BDRFPhongMaterialData& material) { m_materialData = material; }
private:
	uint32_t m_index;
	std::unique_ptr<Texture>	m_textureColor;
	std::unique_ptr<Texture>	m_textureMetallicRoughness;
	ComPtr<ID3D12Resource>		m_materialBuffer;
	PbrMetallicRoughnessData	m_materialData;
	//BDRFPhongMaterialData		m_materialData;
};
	



