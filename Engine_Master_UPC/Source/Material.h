#pragma once
#include "Globals.h"
#include "Resources.h"

namespace tinygltf { class Model; struct Material; struct PbrMetallicRoughness; }
namespace Emeika {
	class Material
	{
	public:
		struct MaterialData {
			Vector4 baseColor;
			BOOL hasColourTexture;  // use BOOL (4 bytes) instead of c++ bool (1 byte) as HLSL bool is 4 bytes long
		};

		struct PhongMaterialData
		{
			Vector4 diffuseColour;
			float    Kd;
			float    Ks;
			float    shininess;
			BOOL     hasDiffuseTex;
		};

		struct BDRFPhongMaterialData {
			Vector3 diffuseColour;
			BOOL hasDiffuseTex;
			Vector3 specularColour;
			float shininess;
		};

		void Load(const tinygltf::Model& model, const tinygltf::PbrMetallicRoughness& material, const char* basePath);
		ComPtr<ID3D12Resource> GetMaterialBuffer() const { return materialBuffer; }
		Texture* GetTexture() const { return _textureColor.get(); }

		BDRFPhongMaterialData& GetMaterial() { return materialData;  }
		void SetMaterial(BDRFPhongMaterialData& material) { materialData = material; }
	private:
		uint32_t index;
		std::unique_ptr<Texture> _textureColor;
		ComPtr<ID3D12Resource> materialBuffer;
		BDRFPhongMaterialData materialData;
	};
}
	



